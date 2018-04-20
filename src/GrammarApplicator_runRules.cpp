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
	CohortSet& cohortset = current->rule_to_cohorts[rsit];
	cohortset.insert(&c);
	return current->valid_rules.insert(rsit);
}

bool GrammarApplicator::updateValidRules(const uint32IntervalVector& rules, uint32IntervalVector& intersects, const uint32_t& hash, Reading& reading) {
	size_t os = intersects.size();
	Grammar::rules_by_tag_t::const_iterator it = grammar->rules_by_tag.find(hash);
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
		const Set& pSet = *(grammar->sets_list[theSet.sets[0]]);
		for (auto iter : pSet.sets) {
			if (unif_sets->count(iter)) {
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
	for (TagList::iterator ot = theTags.begin(); theTags.size() > 1 && ot != theTags.end(); ++ot) {
		TagList::iterator it = ot;
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
	else if (sub_reading == GSR_ANY) {
		subs_any.push_back(Reading());
		Reading* reading = &subs_any.back();
		*reading = *tr;
		reading->next = 0;
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
	else if (sub_reading > 0) {
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
			tr = 0;
		}
		for (int i = ntr; i < sub_reading && tr; ++i) {
			tr = tr->next;
		}
	}
	return tr;
}

#define TRACE                                           \
	do {                                                \
		reading.hit_by.push_back(rule.number);          \
		if (rule.sub_reading == 32767) {                \
			reading_head.hit_by.push_back(rule.number); \
		}                                               \
	} while (0)

#define FILL_TAG_LIST(taglist)                                                      \
	do {                                                                            \
		for (TagList::iterator it = (taglist)->begin(); it != (taglist)->end();) {  \
			if (reading.tags.find((*it)->hash) == reading.tags.end()) {             \
				const Tag* tt = *it;                                                \
				it = (taglist)->erase(it);                                          \
				if (tt->type & T_SPECIAL) {                                         \
					if (regexgrps.second == 0) {                                    \
						regexgrps.second = &regexgrps_store[used_regex];            \
					}                                                               \
					uint32_t stag = doesTagMatchReading(reading, *tt, false, true); \
					if (stag) {                                                     \
						(taglist)->insert(it, single_tags.find(stag)->second);      \
					}                                                               \
				}                                                                   \
				continue;                                                           \
			}                                                                       \
			++it;                                                                   \
		}                                                                           \
	} while (0)

#define APPEND_TAGLIST_TO_READING(taglist, reading)                                  \
	do {                                                                             \
		for (auto tter : (taglist)) {                                                \
			uint32_t hash = tter->hash;                                              \
			while (tter->type & T_VARSTRING) {                                       \
				tter = generateVarstringTag(tter);                                   \
			}                                                                        \
			if (tter->type & T_MAPPING || tter->tag[0] == grammar->mapping_prefix) { \
				mappings->push_back(tter);                                           \
			}                                                                        \
			else {                                                                   \
				hash = addTagToReading((reading), tter);                             \
			}                                                                        \
			if (updateValidRules(rules, intersects, hash, (reading))) {              \
				iter_rules = intersects.find(rule.number);                           \
				iter_rules_end = intersects.end();                                   \
			}                                                                        \
		}                                                                            \
	} while (0)

#define INSERT_TAGLIST_TO_READING(iter, taglist, reading)                          \
	do {                                                                           \
		for (auto tag : (taglist)) {                                               \
			if (tag->type & T_VARSTRING) {                                         \
				tag = generateVarstringTag(tag);                                   \
			}                                                                      \
			if (tag->hash == grammar->tag_any) {                                   \
				break;                                                             \
			}                                                                      \
			if (tag->type & T_MAPPING || tag->tag[0] == grammar->mapping_prefix) { \
				mappings->push_back(tag);                                          \
			}                                                                      \
			else {                                                                 \
				(iter) = (reading).tags_list.insert((iter), tag->hash);            \
				++(iter);                                                          \
			}                                                                      \
			if (updateValidRules(rules, intersects, tag->hash, (reading))) {       \
				iter_rules = intersects.find(rule.number);                         \
				iter_rules_end = intersects.end();                                 \
			}                                                                      \
		}                                                                          \
		reflowReading(reading);                                                    \
	} while (0)

/**
 * Applies the passed rules to the passed SingleWindow.
 *
 * This function is called at least N*M times where N is number of sections in the grammar and M is the number of windows in the input.
 * Possibly many more times, since if a section changes the state of the window the section is run again.
 * Only when no further changes are caused at a level does it progress to next level.
 *
 * The loops in this function are increasingly explosive, despite efforts to contain them.
 * In the https://visl.sdu.dk/cg3_performance.html test data, this function is called 1015 times.
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
	repeat_rule:
		bool rule_did_something = false;
		uint32_t j = (*iter_rules);

		// Check whether this rule is in the allowed rule list from cmdline flag --rule(s)
		if (!valid_rules.empty() && !valid_rules.contains(j)) {
			continue;
		}

		current_rule = grammar->rule_by_number[j];
		const Rule& rule = *(grammar->rule_by_number[j]);
		if (debug_level > 1) {
			std::cerr << "DEBUG: Trying rule " << rule.line << std::endl;
		}

		ticks tstamp(gtimer);
		KEYWORDS type = rule.type;

		if (!apply_mappings && (rule.type == K_MAP || rule.type == K_ADD || rule.type == K_REPLACE)) {
			continue;
		}
		if (!apply_corrections && (rule.type == K_SUBSTITUTE || rule.type == K_APPEND)) {
			continue;
		}
		// If there are parentheses and the rule is marked as only run on the final pass, skip if this is not it.
		if (current.has_enclosures) {
			if ((rule.flags & RF_ENCL_FINAL) && !did_final_enclosure) {
				continue;
			}
			if (did_final_enclosure && !(rule.flags & RF_ENCL_FINAL)) {
				continue;
			}
		}
		if (statistics) {
			tstamp = getticks();
		}

		const Set& set = *(grammar->sets_list[rule.target]);
		grammar->lines = rule.line;

		CohortSet* cohortset = &current.rule_to_cohorts[rule.number];
		if (debug_level > 1) {
			std::cerr << "DEBUG: " << cohortset->size() << "/" << current.cohorts.size() << " = " << double(cohortset->size()) / double(current.cohorts.size()) << std::endl;
		}
		for (CohortSet::const_iterator rocit = cohortset->begin(); rocit != cohortset->end();) {
			Cohort* cohort = *rocit;
			++rocit;

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
			if (cohort->type & CT_REMOVED) {
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

			// Check if on previous runs the rule did not match this cohort, and skip if that is the case.
			// This cache is cleared if any rule causes any state change in the window.
			uint32_t ih = hash_value(rule.number, cohort->global_number);
			if (index_matches(index_ruleCohort_no, ih)) {
				continue;
			}
			index_ruleCohort_no.insert(ih);

			size_t num_active = 0;
			size_t num_iff = 0;
			attach_to = cohort;

			// Assume that Iff rules are really Remove rules, until proven otherwise.
			if (rule.type == K_IFF) {
				type = K_REMOVE;
			}

			bool did_test = false;
			bool test_good = false;
			bool matched_target = false;

			// Older g++ apparently don't check for empty themselves, so we have to.
			if (!readings_plain.empty()) {
				readings_plain.clear();
			}
			if (!subs_any.empty()) {
				subs_any.clear();
			}
			// Varstring capture groups exist on a per-cohort basis, since we may need them for mapping later.
			regexgrps_z.clear();
			regexgrps_c.clear();
			if (!unif_tags_rs.empty()) {
				unif_tags_rs.clear();
			}
			if (!unif_sets_rs.empty()) {
				unif_sets_rs.clear();
			}

			size_t used_regex = 0;
			regexgrps_store.resize(std::max(regexgrps_store.size(), cohort->readings.size()));
			regexgrps_z.reserve(std::max(regexgrps_z.size(), cohort->readings.size()));
			regexgrps_c.reserve(std::max(regexgrps_c.size(), cohort->readings.size()));

			size_t used_unif = 0;
			unif_tags_store.resize(std::max(unif_tags_store.size(), cohort->readings.size()));
			unif_sets_store.resize(std::max(unif_sets_store.size(), cohort->readings.size()));

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

				// The state is stored in the readings themselves, so clear the old states
				reading->matched_target = false;
				reading->matched_tests = false;

				if (reading->mapped && (rule.type == K_MAP || rule.type == K_ADD || rule.type == K_REPLACE)) {
					continue;
				}
				if (reading->noprint && !allow_magic_readings) {
					continue;
				}
				if (reading->immutable && rule.type != K_UNPROTECT) {
					if (type == K_SELECT) {
						reading->matched_target = true;
						reading->matched_tests = true;
					}
					++num_active;
					++num_iff;
					continue;
				}

				// Check if any previous reading of this cohort had the same plain signature, and if so just copy their results
				// This cache is cleared on a per-cohort basis
				if (!(set.type & (ST_SPECIAL | ST_MAPPING | ST_CHILD_UNIFY)) && !readings_plain.empty()) {
					readings_plain_t::const_iterator rpit = readings_plain.find(reading->hash_plain);
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
						}
						continue;
					}
				}

				// Regex capture is done on a per-reading basis, so clear all captured state.
				regexgrps.first = 0;
				regexgrps.second = &regexgrps_store[used_regex];

				// Unification is done on a per-reading basis, so clear all unification state.
				unif_tags = &unif_tags_store[used_unif];
				unif_sets = &unif_sets_store[used_unif];
				unif_tags_rs[reading->hash_plain] = unif_tags;
				unif_sets_rs[reading->hash_plain] = unif_sets;
				unif_tags_rs[reading->hash] = unif_tags;
				unif_sets_rs[reading->hash] = unif_sets;
				++used_unif;

				unif_last_wordform = 0;
				unif_last_baseform = 0;
				unif_last_textual = 0;
				if (!unif_tags->empty()) {
					unif_tags->clear();
				}
				unif_sets_firstrun = true;
				if (!unif_sets->empty()) {
					unif_sets->clear();
				}

				same_basic = reading->hash_plain;
				target = 0;
				mark = cohort;
				uint8_t orz = regexgrps.first;
				// Actually check if the reading is a valid target. First check if rule target matches...
				if (rule.target && doesSetMatchReading(*reading, rule.target, (set.type & (ST_CHILD_UNIFY | ST_SPECIAL)) != 0)) {
					bool regex_prop = true;
					if (orz != regexgrps.first) {
						did_test = false;
						regex_prop = false;
					}
					target = cohort;
					reading->matched_target = true;
					matched_target = true;
					bool good = true;
					// If we didn't already run the contextual tests, run them now.
					if (!did_test) {
						foreach (it, rule.tests) {
							ContextualTest* test = *it;
							if (rule.flags & RF_RESETX || !(rule.flags & RF_REMEMBERX)) {
								mark = cohort;
							}
							seen_barrier = false;
							// Keeps track of where we have been, to prevent infinite recursion in trees with loops
							dep_deep_seen.clear();
							// Reset the counters for which types of CohortIterator we have in play
							std::fill(ci_depths.begin(), ci_depths.end(), static_cast<uint32_t>(0));
							tmpl_cntx.clear();
							// Run the contextual test...
							if (!(test->pos & POS_PASS_ORIGIN) && (no_pass_origin || (test->pos & POS_NO_PASS_ORIGIN))) {
								test_good = (runContextualTest(&current, c, test, 0, cohort) != 0);
							}
							else {
								test_good = (runContextualTest(&current, c, test) != 0);
							}
							if (!test_good) {
								good = test_good;
								if (!statistics) {
									if (it != rule.tests.begin() && !(rule.flags & (RF_REMEMBERX | RF_KEEPORDER))) {
										rule.tests.erase(it);
										rule.tests.push_front(test);
									}
									break;
								}
							}
							did_test = ((set.type & (ST_CHILD_UNIFY | ST_SPECIAL)) == 0 && unif_tags->empty() && unif_sets->empty());
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
								for (size_t j = 0; i < j; ++j) {
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
						++rule.num_match;

						if (regex_prop && i && regexgrps_c.size()) {
							auto it = regexgrps_c.find(cohort->readings[i - 1]->number);
							if (it != regexgrps_c.end()) {
								regexgrps_c.insert(std::make_pair(reading->number, it->second));
								regexgrps_z.insert(std::make_pair(reading->number, regexgrps_z.find(cohort->readings[i - 1]->number)->second));
							}
						}
					}
					else {
						regexgrps.first = orz;
					}
					++num_iff;
				}
				else {
					regexgrps.first = orz;
					++rule.num_fail;
				}
				readings_plain.insert(std::make_pair(reading->hash_plain, reading));

				if (reading != cohort->readings[i]) {
					cohort->readings[i]->matched_target = reading->matched_target;
					cohort->readings[i]->matched_tests = reading->matched_tests;
				}
				if (regexgrps.first) {
					regexgrps_c[reading->number] = regexgrps.second;
					regexgrps_z[reading->number] = regexgrps.first;
					++used_regex;
				}
			}

			// If none of the readings were valid targets, remove this cohort from the rule's possible cohorts.
			if (num_active == 0 && (num_iff == 0 || rule.type != K_IFF)) {
				if (!matched_target) {
					--rocit;                         // We have already incremented rocit earlier, so take one step back...
					rocit = cohortset->erase(rocit); // ...and one step forward again
				}
				continue;
			}

			// All readings were valid targets, which means there is nothing to do for Select or safe Remove rules.
			if (num_active == cohort->readings.size()) {
				if (type == K_SELECT) {
					continue;
				}
				else if (type == K_REMOVE && (!unsafe || (rule.flags & RF_SAFE)) && !(rule.flags & RF_UNSAFE)) {
					continue;
				}
			}

			// Keep track of which readings got removed and selected
			removed.resize(0);
			selected.resize(0);

			bool swap_ac = (attach_to != cohort);
			Cohort* ac_c = cohort;
			if (swap_ac) {
				std::swap(attach_to, cohort);
			}

			// Remember the current state so we can compare later to see if anything has changed
			const size_t state_num_readings = cohort->readings.size();
			const size_t state_num_removed = cohort->deleted.size();
			const size_t state_num_delayed = cohort->delayed.size();
			bool readings_changed = false;

			// This loop acts on the result of the previous loop; letting the rules do their thing on the valid readings.
			for (size_t i = 0; i < cohort->readings.size(); ++i) {
				Reading* tr = get_sub_reading(cohort->readings[i], rule.sub_reading);
				if (!tr) {
					tr = cohort->readings[i];
					tr->matched_target = false;
				}

				Reading& reading = *tr;
				Reading& reading_head = *cohort->readings[i];
				bool good = reading.matched_tests;
				const uint32_t state_hash = reading.hash;

				regexgrps.first = 0;
				regexgrps.second = 0;
				if (regexgrps_c.count(reading.number)) {
					regexgrps.second = regexgrps_c[reading.number];
					regexgrps.first = regexgrps_z[reading.number];
				}

				// Iff needs extra special care; if it is a Remove type and we matched the target, go ahead.
				// If it had matched the tests it would have been Select type.
				if (rule.type == K_IFF && type == K_REMOVE && reading.matched_target) {
					++rule.num_match;
					good = true;
				}

				if (debug_level > 1) {
					std::cerr << "DEBUG: Rule " << rule.line << " fired on reading " << i << std::endl;
				}
				if (dry_run) {
					if (good) {
						TRACE;
					}
					continue;
				}
				unif_tags = 0;
				unif_sets = 0;
				if (unif_tags_rs.count(reading.hash)) {
					unif_tags = unif_tags_rs[reading.hash];
					unif_sets = unif_sets_rs[reading.hash];
				}
				else if (unif_tags_rs.count(reading.hash_plain)) {
					unif_tags = unif_tags_rs[reading.hash_plain];
					unif_sets = unif_sets_rs[reading.hash_plain];
				}

				// Select is also special as it will remove non-matching readings
				if (type == K_SELECT) {
					if (good) {
						selected.push_back(&reading_head);
						index_ruleCohort_no.clear();
						TRACE;
					}
					else {
						removed.push_back(&reading_head);
						index_ruleCohort_no.clear();
						TRACE;
					}
					if (good) {
						if (debug_level > 0) {
							std::cerr << "DEBUG: Rule " << rule.line << " hit cohort " << cohort->local_number << std::endl;
						}
					}
				}
				// Handle all other rule types normally, except that some will break out of the loop as they only make sense to do once per cohort.
				else if (good) {
					if (type == K_REMOVE) {
						if ((rule.flags & RF_UNMAPLAST) && removed.size() == cohort->readings.size() - 1) {
							if (unmapReading(reading, rule.number)) {
								readings_changed = true;
							}
						}
						else {
							removed.push_back(&reading_head);
							TRACE;
						}
						index_ruleCohort_no.clear();
						if (debug_level > 0) {
							std::cerr << "DEBUG: Rule " << rule.line << " hit cohort " << cohort->local_number << std::endl;
						}
					}
					else if (type == K_JUMP) {
						TRACE;
						const Tag* to = getTagList(*rule.maplist).front();
						uint32FlatHashMap::const_iterator it = grammar->anchors.find(to->hash);
						if (it == grammar->anchors.end()) {
							u_fprintf(ux_stderr, "Warning: JUMP on line %u could not find anchor '%S'.\n", rule.line, to->tag.c_str());
						}
						else {
							iter_rules = intersects.lower_bound(it->second);
							--iter_rules;
						}
						goto repeat_rule;
					}
					else if (type == K_PROTECT) {
						TRACE;
						reading.immutable = true;
					}
					else if (type == K_UNPROTECT) {
						TRACE;
						reading.immutable = false;
					}
					else if (type == K_REMVARIABLE) {
						TRACE;
						const TagList names = getTagList(*rule.maplist);
						for (auto tag : names) {
							variables.erase(tag->hash);
							if (rule.flags & RF_OUTPUT) {
								current.variables_output.insert(tag->hash);
							}
							//u_fprintf(ux_stderr, "Info: RemVariable fired for %S.\n", tag->tag.c_str());
						}
						break;
					}
					else if (type == K_SETVARIABLE) {
						TRACE;
						const TagList names = getTagList(*rule.maplist);
						const TagList values = getTagList(*rule.sublist);
						variables[names.front()->hash] = values.front()->hash;
						if (rule.flags & RF_OUTPUT) {
							current.variables_output.insert(names.front()->hash);
						}
						//u_fprintf(ux_stderr, "Info: SetVariable fired for %S.\n", names.front()->tag.c_str());
						break;
					}
					else if (type == K_DELIMIT) {
						delimitAt(current, cohort);
						delimited = true;
						readings_changed = true;
						break;
					}
					else if (type == K_EXTERNAL_ONCE || type == K_EXTERNAL_ALWAYS) {
						if (type == K_EXTERNAL_ONCE && !current.hit_external.insert(rule.line).second) {
							break;
						}

						externals_t::iterator ei = externals.find(rule.varname);
						if (ei == externals.end()) {
							Tag* ext = single_tags.find(rule.varname)->second;
							UErrorCode err = U_ZERO_ERROR;
							u_strToUTF8(&cbuffers[0][0], CG3_BUFFER_SIZE - 1, 0, ext->tag.c_str(), ext->tag.size(), &err);

							Process& es = externals[rule.varname];
							try {
								es.start(&cbuffers[0][0]);
								writeRaw(es, CG3_EXTERNAL_PROTOCOL);
							}
							catch (std::exception& e) {
								u_fprintf(ux_stderr, "Error: External on line %u resulted in error: %s\n", rule.line, e.what());
								CG3Quit(1);
							}
							ei = externals.find(rule.varname);
						}

						pipeOutSingleWindow(current, ei->second);
						pipeInSingleWindow(current, ei->second);

						indexSingleWindow(current);
						readings_changed = true;
						index_ruleCohort_no.clear();
						intersects = current.valid_rules.intersect(rules);
						iter_rules = intersects.find(rule.number);
						iter_rules_end = intersects.end();
						cohortset = &current.rule_to_cohorts[rule.number];
						rocit = cohortset->find(cohort);
						++rocit;
						break;
					}
					else if (type == K_REMCOHORT) {
						for (auto iter : cohort->readings) {
							iter->hit_by.push_back(rule.number);
							iter->deleted = true;
							if (trace) {
								iter->noprint = false;
							}
						}
						// Move any enclosed parentheses to the previous cohort
						if (!cohort->enclosed.empty()) {
							cohort->prev->enclosed.insert(cohort->prev->enclosed.end(), cohort->enclosed.begin(), cohort->enclosed.end());
							cohort->enclosed.clear();
						}
						// Remove the cohort from all rules
						for (auto cs : current.rule_to_cohorts) {
							cs.erase(cohort);
						}
						// Forward all children of this cohort to the parent of this cohort
						// ToDo: Named relations must be erased
						while (!cohort->dep_children.empty()) {
							uint32_t ch = cohort->dep_children.back();
							attachParentChild(*current.parent->cohort_map[cohort->dep_parent], *current.parent->cohort_map[ch], true, true);
							cohort->dep_children.erase(ch);
						}
						cohort->type |= CT_REMOVED;
						cohort->prev->removed.push_back(cohort);
						cohort->detach();
						for (auto cm : current.parent->cohort_map) {
							cm.second->dep_children.erase(cohort->dep_self);
						}
						current.parent->cohort_map.erase(cohort->global_number);
						current.cohorts.erase(current.cohorts.begin() + cohort->local_number);
						foreach (iter, current.cohorts) {
							(*iter)->local_number = std::distance(current.cohorts.begin(), iter);
						}
						gWindow->rebuildCohortLinks();
						// If we just removed the last cohort, add <<< to the new last cohort
						if (cohort->readings.front()->tags.count(endtag)) {
							for (auto r : current.cohorts.back()->readings) {
								addTagToReading(*r, endtag);
								if (updateValidRules(rules, intersects, endtag, *r)) {
									iter_rules = intersects.find(rule.number);
									iter_rules_end = intersects.end();
								}
							}
							index_ruleCohort_no.clear();
							cohortset = &current.rule_to_cohorts[rule.number];
							rocit = cohortset->end();
						}
						else if (cohortset->empty()) {
							rocit = cohortset->end();
						}
						else {
							rocit = cohortset->lower_bound(current.cohorts[cohort->local_number]);
						}
						readings_changed = true;
						break;
					}
					else if (type == K_ADDCOHORT_AFTER || type == K_ADDCOHORT_BEFORE) {
						TRACE;
						index_ruleCohort_no.clear();

						Cohort* cCohort = alloc_cohort(&current);
						cCohort->global_number = gWindow->cohort_counter++;

						Tag* wf = 0;
						std::vector<TagList> readings;
						auto theTags = ss_taglist.get();
						getTagList(*rule.maplist, theTags);

						for (auto tter : *theTags) {
							while (tter->type & T_VARSTRING) {
								tter = generateVarstringTag(tter);
							}
							if (tter->type & T_WORDFORM) {
								cCohort->wordform = tter;
								wf = tter;
								continue;
							}
							if (!wf) {
								u_fprintf(ux_stderr, "Error: There must be a wordform before any other tags in ADDCOHORT on line %u before input line %u.\n", rule.line, numLines);
								CG3Quit(1);
							}
							if (tter->type & T_BASEFORM) {
								readings.resize(readings.size() + 1);
								readings.back().push_back(wf);
							}
							if (readings.empty()) {
								u_fprintf(ux_stderr, "Error: There must be a baseform after the wordform in ADDCOHORT on line %u before input line %u.\n", rule.line, numLines);
								CG3Quit(1);
							}
							readings.back().push_back(tter);
						}

						for (auto rit : readings) {
							Reading* cReading = alloc_reading(cCohort);
							++numReadings;
							insert_if_exists(cReading->parent->possible_sets, grammar->sets_any);
							cReading->hit_by.push_back(rule.number);
							cReading->noprint = false;
							TagList mappings;
							for (auto tter : rit) {
								uint32_t hash = tter->hash;
								while (tter->type & T_VARSTRING) {
									tter = generateVarstringTag(tter);
								}
								if (tter->type & T_MAPPING || tter->tag[0] == grammar->mapping_prefix) {
									mappings.push_back(tter);
								}
								else {
									hash = addTagToReading(*cReading, hash);
								}
								if (updateValidRules(rules, intersects, hash, *cReading)) {
									iter_rules = intersects.find(rule.number);
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

						if (cCohort->readings.empty()) {
							initEmptyCohort(*cCohort);
							if (trace) {
								auto r = cCohort->readings.front();
								r->hit_by.push_back(rule.number);
								r->noprint = false;
							}
						}

						CohortSet cohorts;
						if (rule.childset1) {
							for (auto iter : current.cohorts) {
								// Always consider the target cohort a match
								if (iter->global_number == cohort->global_number) {
									cohorts.insert(iter);
								}
								else if (iter->dep_parent == cohort->global_number && doesSetMatchCohortNormal(*iter, rule.childset1)) {
									cohorts.insert(iter);
								}
							}
							CohortSet more;
							for (auto iter : current.cohorts) {
								for (auto cht : cohorts) {
									// Do not grab the whole tree from the root, in case WithChild is not (*)
									if (cht->global_number == cohort->global_number) {
										continue;
									}
									if (isChildOf(iter, cht)) {
										more.insert(iter);
									}
								}
							}
							cohorts.insert(more.begin(), more.end());
						}
						else {
							cohorts.insert(cohort);
						}

						if (type == K_ADDCOHORT_BEFORE) {
							current.cohorts.insert(current.cohorts.begin() + cohorts.front()->local_number, cCohort);
						}
						else {
							current.cohorts.insert(current.cohorts.begin() + cohorts.back()->local_number + 1, cCohort);
						}

						foreach (iter, current.cohorts) {
							(*iter)->local_number = std::distance(current.cohorts.begin(), iter);
						}
						// If the new cohort is now the last cohort, add <<< to it and remove <<< from previous last cohort
						if (current.cohorts.back() == cCohort) {
							for (auto r : current.cohorts[current.cohorts.size() - 2]->readings) {
								delTagFromReading(*r, endtag);
							}
							for (auto r : current.cohorts.back()->readings) {
								addTagToReading(*r, endtag);
								if (updateValidRules(rules, intersects, endtag, *r)) {
									iter_rules = intersects.find(rule.number);
									iter_rules_end = intersects.end();
								}
							}
						}
						gWindow->rebuildCohortLinks();
						indexSingleWindow(current);
						readings_changed = true;

						cohortset = &current.rule_to_cohorts[rule.number];
						rocit = cohortset->find(cohort);
						++rocit;
						break;
					}
					else if (rule.type == K_SPLITCOHORT) {
						index_ruleCohort_no.clear();

						std::vector<std::pair<Cohort*, std::vector<TagList>>> cohorts;

						auto theTags = ss_taglist.get();
						getTagList(*rule.maplist, theTags);

						Tag* wf = 0;
						for (auto tter : *theTags) {
							if (tter->type & T_WORDFORM) {
								cohorts.resize(cohorts.size() + 1);
								cohorts.back().first = alloc_cohort(&current);
								cohorts.back().first->global_number = gWindow->cohort_counter++;
								wf = tter;
								while (wf->type & T_VARSTRING) {
									wf = generateVarstringTag(wf);
								}
								cohorts.back().first->wordform = wf;
								continue;
							}
							if (!wf) {
								u_fprintf(ux_stderr, "Error: There must be a wordform before any other tags in SPLITCOHORT on line %u before input line %u.\n", rule.line, numLines);
								CG3Quit(1);
							}
						}

						uint32_t rel_trg = DEP_NO_PARENT;
						std::vector<std::pair<uint32_t, uint32_t>> cohort_dep(cohorts.size());
						cohort_dep.front().second = DEP_NO_PARENT;
						cohort_dep.back().first = DEP_NO_PARENT;
						cohort_dep.back().second = cohort_dep.size() - 1;
						for (size_t i = 1; i < cohort_dep.size() - 1; ++i) {
							cohort_dep[i].second = i;
						}

						size_t i = 0;
						std::vector<TagList>* readings = &cohorts.front().second;
						Tag* bf = 0;
						for (auto tter : *theTags) {
							if (tter->type & T_WORDFORM) {
								++i;
								bf = 0;
								continue;
							}
							if (tter->type & T_BASEFORM) {
								readings = &cohorts[i - 1].second;
								readings->resize(readings->size() + 1);
								readings->back().push_back(cohorts[i - 1].first->wordform);
								bf = tter;
							}
							if (!bf) {
								u_fprintf(ux_stderr, "Error: There must be a baseform after the wordform in SPLITCOHORT on line %u before input line %u.\n", rule.line, numLines);
								CG3Quit(1);
							}

							UChar dep_self[12] = {};
							UChar dep_parent[12] = {};
							if (u_sscanf(tter->tag.c_str(), "%[0-9cd]->%[0-9pm]", &dep_self, &dep_parent) == 2) {
								if (dep_self[0] == 'c' || dep_self[0] == 'd') {
									cohort_dep[i - 1].first = DEP_NO_PARENT;
									if (rel_trg == DEP_NO_PARENT) {
										rel_trg = i - 1;
									}
								}
								else if (u_sscanf(dep_self, "%i", &cohort_dep[i - 1].first) != 1) {
									u_fprintf(ux_stderr, "Error: SPLITCOHORT dependency mapping dep_self was not valid on line %u before input line %u.\n", rule.line, numLines);
									CG3Quit(1);
								}
								if (dep_parent[0] == 'p' || dep_parent[0] == 'm') {
									cohort_dep[i - 1].second = DEP_NO_PARENT;
								}
								else if (u_sscanf(dep_parent, "%i", &cohort_dep[i - 1].second) != 1) {
									u_fprintf(ux_stderr, "Error: SPLITCOHORT dependency mapping dep_parent was not valid on line %u before input line %u.\n", rule.line, numLines);
									CG3Quit(1);
								}
								continue;
							}
							if (tter->tag.size() == 3 && tter->tag[0] == 'R' && tter->tag[1] == ':' && tter->tag[2] == '*') {
								rel_trg = i - 1;
								continue;
							}
							readings->back().push_back(tter);
						}

						if (rel_trg == DEP_NO_PARENT) {
							rel_trg = cohorts.size() - 1;
						}

						for (size_t i = 0; i < cohorts.size(); ++i) {
							Cohort* cCohort = cohorts[i].first;
							readings = &cohorts[i].second;

							for (auto tags : *readings) {
								Reading* cReading = alloc_reading(cCohort);
								++numReadings;
								insert_if_exists(cReading->parent->possible_sets, grammar->sets_any);
								cReading->hit_by.push_back(rule.number);
								cReading->noprint = false;
								TagList mappings;

								for (size_t i = 0; i < tags.size(); ++i) {
									if (tags[i]->hash == grammar->tag_any) {
										uint32Vector& nt = cohort->readings.front()->tags_list;
										if (nt.size() <= 2) {
											continue;
										}
										tags.reserve(tags.size() + nt.size() - 2);
										tags[i] = single_tags[nt[2]];
										for (size_t j = 3, k = 1; j < nt.size(); ++j) {
											if (single_tags[nt[j]]->type & T_DEPENDENCY) {
												continue;
											}
											tags.insert(tags.begin() + i + k, single_tags[nt[j]]);
											++k;
										}
									}
								}

								for (auto tter : tags) {
									uint32_t hash = tter->hash;
									while (tter->type & T_VARSTRING) {
										tter = generateVarstringTag(tter);
									}
									if (tter->type & T_MAPPING || tter->tag[0] == grammar->mapping_prefix) {
										mappings.push_back(tter);
									}
									else {
										hash = addTagToReading(*cReading, hash);
									}
									if (updateValidRules(rules, intersects, hash, *cReading)) {
										iter_rules = intersects.find(rule.number);
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

							current.cohorts.insert(current.cohorts.begin() + cohort->local_number + i + 1, cCohort);
						}

						for (size_t i = 0; i < cohorts.size(); ++i) {
							Cohort* cCohort = cohorts[i].first;

							if (cohort_dep[i].first == DEP_NO_PARENT) {
								while (!cohort->dep_children.empty()) {
									uint32_t ch = cohort->dep_children.back();
									attachParentChild(*cCohort, *current.parent->cohort_map[ch], true, true);
									cohort->dep_children.erase(ch); // Just in case the attachment can't be made for some reason
								}
							}

							if (cohort_dep[i].second == DEP_NO_PARENT) {
								if (current.parent->cohort_map.count(cohort->dep_parent)) {
									attachParentChild(*current.parent->cohort_map[cohort->dep_parent], *cCohort, true, true);
								}
							}
							else {
								attachParentChild(*current.parent->cohort_map[cohorts.front().first->global_number + cohort_dep[i].second - 1], *cCohort, true, true);
							}

							// Re-attach all named relations to the dependency tail or R:* cohort
							if (rel_trg == i && (cohort->type & CT_RELATED)) {
								cCohort->type |= CT_RELATED;
								cCohort->relations.swap(cohort->relations);

								std::pair<SingleWindow**, size_t> swss[3] = {
									std::make_pair(&gWindow->previous[0], gWindow->previous.size()),
									std::make_pair(&gWindow->current, static_cast<size_t>(1)),
									std::make_pair(&gWindow->next[0], gWindow->next.size()),
								};
								for (size_t w = 0; w < 3; ++w) {
									for (size_t sw = 0; sw < swss[w].second; ++sw) {
										for (auto ch : swss[w].first[sw]->cohorts) {
											for (auto& rel : ch->relations) {
												if (rel.second.count(cohort->global_number)) {
													rel.second.erase(cohort->global_number);
													rel.second.insert(cCohort->global_number);
												}
											}
										}
									}
								}
							}
						}

						// Remove the source cohort
						for (auto iter : cohort->readings) {
							iter->hit_by.push_back(rule.number);
							iter->deleted = true;
						}
						// Move any enclosed parentheses to the previous cohort
						if (!cohort->enclosed.empty()) {
							cohort->prev->enclosed.insert(cohort->prev->enclosed.end(), cohort->enclosed.begin(), cohort->enclosed.end());
							cohort->enclosed.clear();
						}
						cohort->type |= CT_REMOVED;
						cohort->prev->removed.push_back(cohort);
						cohort->detach();
						for (auto cm : current.parent->cohort_map) {
							cm.second->dep_children.erase(cohort->dep_self);
						}
						current.parent->cohort_map.erase(cohort->global_number);
						current.cohorts.erase(current.cohorts.begin() + cohort->local_number);

						// Reindex and rebuild the window
						foreach (iter, current.cohorts) {
							(*iter)->local_number = std::distance(current.cohorts.begin(), iter);
						}
						gWindow->rebuildCohortLinks();
						indexSingleWindow(current);
						readings_changed = true;

						cohortset = &current.rule_to_cohorts[rule.number];
						rocit = cohortset->find(current.cohorts[cohort->local_number]);
						if (rocit != cohortset->end()) {
							++rocit;
						}
						break;
					}
					else if (rule.type == K_ADD || rule.type == K_MAP) {
						index_ruleCohort_no.clear();
						TRACE;
						reading.noprint = false;
						auto mappings = ss_taglist.get();
						auto theTags = ss_taglist.get();
						getTagList(*rule.maplist, theTags);

						bool did_insert = false;
						if (rule.childset1) {
							auto spot_tags = ss_taglist.get();
							getTagList(*grammar->sets_list[rule.childset1], spot_tags);
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
									break;
								}
							}
							if (rule.flags & RF_AFTER) {
								std::advance(it, spot_tags->size());
							}
							if (it != reading.tags_list.end()) {
								INSERT_TAGLIST_TO_READING(it, *theTags, reading);
								did_insert = true;
							}
						}

						if (!did_insert) {
							APPEND_TAGLIST_TO_READING(*theTags, reading);
						}
						if (!mappings->empty()) {
							splitMappings(mappings, *cohort, reading, rule.type == K_MAP);
						}
						if (rule.type == K_MAP) {
							reading.mapped = true;
						}
						if (reading.hash != state_hash) {
							readings_changed = true;
						}
					}
					else if (rule.type == K_UNMAP) {
						if (unmapReading(reading, rule.number)) {
							index_ruleCohort_no.clear();
							readings_changed = true;
						}
					}
					else if (rule.type == K_REPLACE) {
						index_ruleCohort_no.clear();
						TRACE;
						reading.noprint = false;
						reading.tags_list.clear();
						reading.tags_list.push_back(cohort->wordform->hash);
						reading.tags_list.push_back(reading.baseform);
						reflowReading(reading);
						auto mappings = ss_taglist.get();
						auto theTags = ss_taglist.get();
						getTagList(*rule.maplist, theTags);

						APPEND_TAGLIST_TO_READING(*theTags, reading);

						if (!mappings->empty()) {
							splitMappings(mappings, *cohort, reading, true);
						}
						if (reading.hash != state_hash) {
							readings_changed = true;
						}
					}
					else if (rule.type == K_SUBSTITUTE) {
						// ToDo: Check whether this substitution will do nothing at all to the end result
						// ToDo: Not actually...instead, test whether any reading in the cohort already is the end result

						auto theTags = ss_taglist.get();
						getTagList(*rule.sublist, theTags);

						// Modify the list of tags to remove to be the actual list of tags present, including matching regex and icase tags
						FILL_TAG_LIST(theTags);

						// Perform the tag removal, remembering the position of the final removed tag for use as insertion spot
						size_t tpos = std::numeric_limits<size_t>::max();
						bool plain = true;
						for (size_t i = 0; i < reading.tags_list.size();) {
							auto& remter = reading.tags_list[i];

							if (plain && remter == (*theTags->begin())->hash) {
								if (reading.baseform == remter) {
									reading.baseform = 0;
								}
								remter = substtag;
								tpos = i;
								for (size_t j = 1; j < theTags->size() && i < reading.tags_list.size(); ++j, ++i) {
									auto& remter = reading.tags_list[i];
									auto tter = (*theTags)[j]->hash;
									if (remter != tter) {
										plain = false;
										break;
									}
									reading.tags_list.erase(reading.tags_list.begin() + i);
									reading.tags.erase(tter);
									if (reading.baseform == tter) {
										reading.baseform = 0;
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
								reading.tags.erase(tter->hash);
								if (reading.baseform == tter->hash) {
									reading.baseform = 0;
								}
							}

							++i;
						}

						// Should Substitute really do nothing if no tags were removed? 2013-10-21, Eckhard says this is expected behavior.
						if (tpos != std::numeric_limits<size_t>::max()) {
							if (!plain) {
								for (size_t i = 0; i < reading.tags_list.size() && i < tpos;) {
									if (reading.tags_list[i] == substtag) {
										reading.tags_list.erase(reading.tags_list.begin() + i);
										--tpos;
									}
									else {
										++i;
									}
								}
							}

							Tag* wf = 0;
							index_ruleCohort_no.clear();
							TRACE;
							reading.noprint = false;
							if (tpos >= reading.tags_list.size()) {
								tpos = reading.tags_list.size() - 1;
							}
							++tpos;
							auto mappings = ss_taglist.get();
							auto theTags = ss_taglist.get();
							getTagList(*rule.maplist, theTags);

							for (size_t i = 0; i < reading.tags_list.size();) {
								if (reading.tags_list[i] == substtag) {
									reading.tags_list.erase(reading.tags_list.begin() + i);
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
											reading.tags_list.insert(reading.tags_list.begin() + tpos, tag->hash);
											++tpos;
										}
										if (updateValidRules(rules, intersects, tag->hash, reading)) {
											iter_rules = intersects.find(rule.number);
											iter_rules_end = intersects.end();
										}
									}
								}
								else {
									++i;
								}
							}
							reflowReading(reading);

							if (!mappings->empty()) {
								splitMappings(mappings, *cohort, reading, true);
							}
							if (wf && wf != reading.parent->wordform) {
								for (auto r : reading.parent->readings) {
									delTagFromReading(*r, reading.parent->wordform);
									addTagToReading(*r, wf);
								}
								for (auto r : reading.parent->deleted) {
									delTagFromReading(*r, reading.parent->wordform);
									addTagToReading(*r, wf);
								}
								for (auto r : reading.parent->delayed) {
									delTagFromReading(*r, reading.parent->wordform);
									addTagToReading(*r, wf);
								}
								reading.parent->wordform = wf;
								for (auto r : grammar->wf_rules) {
									if (doesWordformsMatch(wf, r->wordform)) {
										current.rule_to_cohorts[r->number].insert(cohort);
										intersects.insert(r->number);
									}
									else {
										current.rule_to_cohorts[r->number].erase(cohort);
									}
								}
								updateValidRules(rules, intersects, wf->hash, reading);
								iter_rules = intersects.find(rule.number);
								iter_rules_end = intersects.end();
							}
						}
						if (reading.hash != state_hash) {
							readings_changed = true;
						}
					}
					else if (rule.type == K_APPEND) {
						index_ruleCohort_no.clear();

						Tag* bf = 0;
						std::vector<TagList> readings;
						auto theTags = ss_taglist.get();
						getTagList(*rule.maplist, theTags);

						for (auto tter : *theTags) {
							while (tter->type & T_VARSTRING) {
								tter = generateVarstringTag(tter);
							}
							if (tter->type & T_BASEFORM) {
								bf = tter;
								readings.resize(readings.size() + 1);
							}
							if (bf == 0) {
								u_fprintf(ux_stderr, "Error: There must be a baseform before any other tags in APPEND on line %u.\n", rule.line);
								CG3Quit(1);
							}
							readings.back().push_back(tter);
						}

						for (auto rit : readings) {
							Reading* cReading = alloc_reading(cohort);
							++numReadings;
							insert_if_exists(cReading->parent->possible_sets, grammar->sets_any);
							addTagToReading(*cReading, cohort->wordform);
							cReading->hit_by.push_back(rule.number);
							cReading->noprint = false;
							TagList mappings;
							for (auto tter : rit) {
								uint32_t hash = tter->hash;
								while (tter->type & T_VARSTRING) {
									tter = generateVarstringTag(tter);
								}
								if (tter->type & T_MAPPING || tter->tag[0] == grammar->mapping_prefix) {
									mappings.push_back(tter);
								}
								else {
									hash = addTagToReading(*cReading, tter);
								}
								if (updateValidRules(rules, intersects, hash, *cReading)) {
									iter_rules = intersects.find(rule.number);
									iter_rules_end = intersects.end();
								}
							}
							if (!mappings.empty()) {
								splitMappings(mappings, *cohort, *cReading);
							}
							cohort->appendReading(cReading);
						}

						if (cohort->readings.size() > 1) {
							foreach (rit, cohort->readings) {
								if ((*rit)->noprint) {
									delete *rit;
									rit = cohort->readings.erase(rit);
									rit_end = cohort->readings.end();
								}
							}
						}

						readings_changed = true;
						break;
					}
					else if (rule.type == K_COPY) {
						// ToDo: Maybe just goto Substitute directly?
						Reading* cReading = cohort->allocateAppendReading(reading_head);
						++numReadings;
						index_ruleCohort_no.clear();
						cReading->hit_by.push_back(rule.number);
						cReading->noprint = false;

						if (rule.sublist) {
							auto excepts = ss_taglist.get();
							getTagList(*rule.sublist, excepts);
							FILL_TAG_LIST(excepts);
							for (auto tter : *excepts) {
								delTagFromReading(*cReading, tter);
							}
						}

						auto mappings = ss_taglist.get();
						auto theTags = ss_taglist.get();
						getTagList(*rule.maplist, theTags);

						bool did_insert = false;
						if (rule.childset1) {
							auto spot_tags = ss_taglist.get();
							getTagList(*grammar->sets_list[rule.childset1], spot_tags);
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
							if (rule.flags & RF_AFTER) {
								std::advance(it, spot_tags->size());
							}
							if (it != cReading->tags_list.end()) {
								INSERT_TAGLIST_TO_READING(it, *theTags, *cReading);
								did_insert = true;
							}
						}

						if (!did_insert) {
							APPEND_TAGLIST_TO_READING(*theTags, *cReading);
						}
						if (!mappings->empty()) {
							splitMappings(mappings, *cohort, *cReading, true);
						}
						readings_changed = true;
						reflowReading(*cReading);
					}
					else if (type == K_SETPARENT || type == K_SETCHILD) {
						int32_t orgoffset = rule.dep_target->offset;
						auto seen_targets = ss_u32sv.get();

						bool attached = false;
						Cohort* target = cohort;
						while (!attached) {
							auto utags = ss_utags.get();
							auto usets = ss_u32sv.get();
							*utags = *unif_tags;
							*usets = *unif_sets;

							Cohort* attach = 0;
							seen_targets->insert(target->global_number);
							dep_deep_seen.clear();
							tmpl_cntx.clear();
							attach_to = 0;
							seen_barrier = false;
							if (runContextualTest(target->parent, target->local_number, rule.dep_target, &attach) && attach) {
								bool break_after = seen_barrier || (rule.flags & RF_NEAREST);
								if (attach_to) {
									attach = attach_to;
								}
								bool good = true;
								for (auto it : rule.dep_tests) {
									mark = attach;
									dep_deep_seen.clear();
									tmpl_cntx.clear();
									test_good = (runContextualTest(attach->parent, attach->local_number, it) != 0);
									if (!test_good) {
										good = test_good;
										break;
									}
								}
								if (good) {
									swapper<Cohort*> sw((rule.flags & RF_REVERSE) != 0, attach, cohort);
									if (type == K_SETPARENT) {
										attached = attachParentChild(*attach, *cohort, (rule.flags & RF_ALLOWLOOP) != 0, (rule.flags & RF_ALLOWCROSS) != 0);
									}
									else {
										attached = attachParentChild(*cohort, *attach, (rule.flags & RF_ALLOWLOOP) != 0, (rule.flags & RF_ALLOWCROSS) != 0);
									}
									if (attached) {
										index_ruleCohort_no.clear();
										TRACE;
										reading.noprint = false;
										has_dep = true;
										readings_changed = true;
										break;
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
								if (!attached) {
									// Did not successfully attach due to loop restrictions; look onwards from here
									target = attach;
									unif_tags->swap(utags);
									unif_sets->swap(usets);
									if (rule.dep_target->offset != 0) {
										// Temporarily set offset to +/- 1
										rule.dep_target->offset = ((rule.dep_target->offset < 0) ? -1 : 1);
									}
								}
							}
							else {
								break;
							}
						}
						rule.dep_target->offset = orgoffset;
						break;
					}
					else if (type == K_MOVE_AFTER || type == K_MOVE_BEFORE || type == K_SWITCH) {
						// ToDo: ** tests will not correctly work for MOVE/SWITCH; cannot move cohorts between windows
						Cohort* attach = 0;
						dep_deep_seen.clear();
						tmpl_cntx.clear();
						attach_to = 0;
						if (runContextualTest(&current, c, rule.dep_target, &attach) && attach && cohort->parent == attach->parent) {
							if (attach_to) {
								attach = attach_to;
							}
							bool good = true;
							for (auto it : rule.dep_tests) {
								mark = attach;
								dep_deep_seen.clear();
								tmpl_cntx.clear();
								test_good = (runContextualTest(attach->parent, attach->local_number, it) != 0);
								if (!test_good) {
									good = test_good;
									break;
								}
							}

							if (!good || cohort == attach || cohort->local_number == 0) {
								break;
							}

							swapper<Cohort*> sw((rule.flags & RF_REVERSE) != 0, attach, cohort);

							if (type == K_SWITCH) {
								if (attach->local_number == 0) {
									break;
								}
								current.cohorts[cohort->local_number] = attach;
								current.cohorts[attach->local_number] = cohort;
								for (auto iter : cohort->readings) {
									iter->hit_by.push_back(rule.number);
								}
								for (auto iter : attach->readings) {
									iter->hit_by.push_back(rule.number);
								}
							}
							else {
								CohortSet edges;
								if (rule.childset2) {
									for (auto iter : current.cohorts) {
										// Always consider the anchor cohort a match
										if (iter->global_number == attach->global_number) {
											edges.insert(iter);
										}
										else if (iter->dep_parent == attach->global_number && doesSetMatchCohortNormal(*iter, rule.childset2)) {
											edges.insert(iter);
										}
									}
									CohortSet more;
									for (auto iter : current.cohorts) {
										for (auto edge : edges) {
											// Do not grab the whole tree from the root, in case WithChild is not (*)
											if (edge->global_number == attach->global_number) {
												continue;
											}
											if (isChildOf(iter, edge)) {
												more.insert(iter);
											}
										}
									}
									edges.insert(more.begin(), more.end());
								}
								else {
									edges.insert(attach);
								}

								CohortSet cohorts;
								if (rule.childset1) {
									for (auto iter : current.cohorts) {
										// Always consider the target cohort a match
										if (iter->global_number == cohort->global_number) {
											cohorts.insert(iter);
										}
										else if (iter->dep_parent == cohort->global_number && doesSetMatchCohortNormal(*iter, rule.childset1)) {
											cohorts.insert(iter);
										}
									}
									CohortSet more;
									for (auto iter : current.cohorts) {
										for (auto cht : cohorts) {
											// Do not grab the whole tree from the root, in case WithChild is not (*)
											if (cht->global_number == cohort->global_number) {
												continue;
											}
											if (isChildOf(iter, cht)) {
												more.insert(iter);
											}
										}
									}
									cohorts.insert(more.begin(), more.end());
								}
								else {
									cohorts.insert(cohort);
								}

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
									break;
								}

								reverse_foreach (iter, cohorts) {
									current.cohorts.erase(current.cohorts.begin() + (*iter)->local_number);
								}

								foreach (iter, current.cohorts) {
									(*iter)->local_number = std::distance(current.cohorts.begin(), iter);
								}

								for (auto iter : edges) {
									if (iter->parent != cohort->parent) {
										u_fprintf(ux_stderr, "Error: Move/Switch on line %u tried to move across window boundaries.\n", rule.line);
										CG3Quit(1);
									}
									for (auto cohort : cohorts) {
										if (iter == cohort) {
											u_fprintf(ux_stderr, "Error: Move/Switch on line %u tried to move to a removed position.\n", rule.line);
											CG3Quit(1);
										}
									}
								}

								uint32_t spot = 0;
								if (type == K_MOVE_BEFORE) {
									spot = edges.front()->local_number;
									if (spot == 0) {
										spot = 1;
									}
								}
								else if (type == K_MOVE_AFTER) {
									spot = edges.back()->local_number + 1;
								}

								if (spot > current.cohorts.size()) {
									u_fprintf(ux_stderr, "Error: Move/Switch on line %u tried to move out of bounds.\n", rule.line);
									CG3Quit(1);
								}

								while (!cohorts.empty()) {
									for (auto iter : cohorts.back()->readings) {
										iter->hit_by.push_back(rule.number);
									}
									current.cohorts.insert(current.cohorts.begin() + spot, cohorts.back());
									cohorts.pop_back();
								}
							}
							foreach (iter, current.cohorts) {
								(*iter)->local_number = std::distance(current.cohorts.begin(), iter);
							}
							gWindow->rebuildCohortLinks();
							readings_changed = true;
							break;
						}
					}
					else if (type == K_ADDRELATION || type == K_SETRELATION || type == K_REMRELATION) {
						Cohort* attach = 0;
						dep_deep_seen.clear();
						attach_to = 0;
						// ToDo: Maybe allow SetRelation family to scan on after failed tests?
						if (runContextualTest(&current, c, rule.dep_target, &attach) && attach) {
							if (attach_to) {
								attach = attach_to;
							}
							bool good = true;
							for (auto it : rule.dep_tests) {
								mark = attach;
								dep_deep_seen.clear();
								test_good = (runContextualTest(attach->parent, attach->local_number, it) != 0);
								if (!test_good) {
									good = test_good;
									break;
								}
							}
							if (good) {
								swapper<Cohort*> sw((rule.flags & RF_REVERSE) != 0, attach, cohort);
								bool rel_did_anything = false;
								auto theTags = ss_taglist.get();
								getTagList(*rule.maplist, theTags);

								for (auto tter : *theTags) {
									while (tter->type & T_VARSTRING) {
										tter = generateVarstringTag(tter);
									}
									if (type == K_ADDRELATION) {
										attach->type |= CT_RELATED;
										cohort->type |= CT_RELATED;
										rel_did_anything |= cohort->addRelation(tter->hash, attach->global_number);
									}
									else if (type == K_SETRELATION) {
										attach->type |= CT_RELATED;
										cohort->type |= CT_RELATED;
										rel_did_anything |= cohort->setRelation(tter->hash, attach->global_number);
									}
									else {
										rel_did_anything |= cohort->remRelation(tter->hash, attach->global_number);
									}
								}
								if (rel_did_anything) {
									index_ruleCohort_no.clear();
									TRACE;
									reading.noprint = false;
									readings_changed = true;
								}
							}
						}
						break;
					}
					else if (type == K_ADDRELATIONS || type == K_SETRELATIONS || type == K_REMRELATIONS) {
						Cohort* attach = 0;
						dep_deep_seen.clear();
						tmpl_cntx.clear();
						attach_to = 0;
						if (runContextualTest(&current, c, rule.dep_target, &attach) && attach) {
							if (attach_to) {
								attach = attach_to;
							}
							bool good = true;
							for (auto it : rule.dep_tests) {
								mark = attach;
								dep_deep_seen.clear();
								tmpl_cntx.clear();
								test_good = (runContextualTest(attach->parent, attach->local_number, it) != 0);
								if (!test_good) {
									good = test_good;
									break;
								}
							}
							if (good) {
								swapper<Cohort*> sw((rule.flags & RF_REVERSE) != 0, attach, cohort);
								bool rel_did_anything = false;

								auto sublist = ss_taglist.get();
								getTagList(*rule.sublist, sublist);

								auto maplist = ss_taglist.get();
								getTagList(*rule.maplist, maplist);

								for (auto tter : *maplist) {
									while (tter->type & T_VARSTRING) {
										tter = generateVarstringTag(tter);
									}
									if (type == K_ADDRELATIONS) {
										cohort->type |= CT_RELATED;
										rel_did_anything |= cohort->addRelation(tter->hash, attach->global_number);
									}
									else if (type == K_SETRELATIONS) {
										cohort->type |= CT_RELATED;
										rel_did_anything |= cohort->setRelation(tter->hash, attach->global_number);
									}
									else {
										rel_did_anything |= cohort->remRelation(tter->hash, attach->global_number);
									}
								}
								for (auto tter : *sublist) {
									while (tter->type & T_VARSTRING) {
										tter = generateVarstringTag(tter);
									}
									if (type == K_ADDRELATIONS) {
										attach->type |= CT_RELATED;
										rel_did_anything |= attach->addRelation(tter->hash, cohort->global_number);
									}
									else if (type == K_SETRELATIONS) {
										attach->type |= CT_RELATED;
										rel_did_anything |= attach->setRelation(tter->hash, cohort->global_number);
									}
									else {
										rel_did_anything |= attach->remRelation(tter->hash, cohort->global_number);
									}
								}
								if (rel_did_anything) {
									index_ruleCohort_no.clear();
									TRACE;
									reading.noprint = false;
									readings_changed = true;
								}
							}
						}
					}
				}
			}

			// We've marked all readings for removal, so check if the rule is unsafe and undo if not.
			// Iff rules can slip through the previous checks and come all the way here.
			if (type == K_REMOVE && removed.size() == cohort->readings.size() && (!unsafe || (rule.flags & RF_SAFE)) && !(rule.flags & RF_UNSAFE)) {
				removed.clear();
			}

			// Actually remove readings from the cohort
			if (!removed.empty()) {
				if (rule.flags & RF_DELAYED) {
					cohort->delayed.insert(cohort->delayed.end(), removed.begin(), removed.end());
				}
				else {
					cohort->deleted.insert(cohort->deleted.end(), removed.begin(), removed.end());
				}
				size_t oz = cohort->readings.size();
				while (!removed.empty()) {
					removed.back()->deleted = true;
					for (size_t i = 0; i < oz; ++i) {
						if (cohort->readings[i] == removed.back()) {
							--oz;
							std::swap(cohort->readings[i], cohort->readings[oz]);
						}
					}
					removed.pop_back();
				}
				cohort->readings.resize(oz);
				if (debug_level > 0) {
					std::cerr << "DEBUG: Rule " << rule.line << " hit cohort " << cohort->local_number << std::endl;
				}
			}
			// If there are any selected cohorts, just swap them in...
			if (!selected.empty()) {
				cohort->readings.swap(selected);
			}

			if (cohort->readings.empty()) {
				initEmptyCohort(*cohort);
			}

			// Cohort state has changed, so mark that the section did something
			if (state_num_readings != cohort->readings.size() || state_num_removed != cohort->deleted.size() || state_num_delayed != cohort->delayed.size() || readings_changed) {
				if (!(rule.flags & RF_NOITERATE) && section_max_count != 1) {
					section_did_something = true;
				}
				rule_did_something = true;
				cohort->type &= ~CT_NUM_CURRENT;
			}

			if (delimited) {
				break;
			}

			if (swap_ac) {
				cohort = ac_c;
				if (cohortset->empty()) {
					rocit = cohortset->end();
				}
				else {
					rocit = cohortset->find(current.cohorts[cohort->local_number]);
					if (rocit != cohortset->end()) {
						++rocit;
					}
				}
			}
		}

		if (statistics) {
			ticks tmp = getticks();
			rule.total_time += elapsed(tmp, tstamp);
		}

		if (rule_did_something) {
			if (trace_rules.contains(rule.line)) {
				retval |= RV_TRACERULE;
			}
		}
		if (delimited) {
			break;
		}
		if (rule_did_something && (rule.flags & RF_REPEAT)) {
			index_ruleCohort_no.clear();
			goto repeat_rule;
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
		RSType::iterator iter = runsections.begin();
		RSType::iterator iter_end = runsections.end();
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
					u_fprintf(ux_stderr, " %S", tag.c_str());
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

	for (auto vit : current->variables_set) {
		variables[vit.first] = vit.second;
	}
	for (auto vit : current->variables_rem) {
		variables.erase(vit);
	}

	if (has_dep) {
		reflowDependencyWindow();
		gWindow->dep_map.clear();
		gWindow->dep_window.clear();
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
			Grammar::parentheses_t::const_iterator p = grammar->parentheses.find(c->is_pleft);
			if (p != grammar->parentheses.end()) {
				CohortVector::iterator right = iter.base();
				--right;
				--right;
				c = *right;
				++right;
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
				if (!found) {
					encs.clear();
				}
				else {
					CohortVector::iterator left = iter.base();
					--left;
					uint32_t lc = (*left)->local_number;
					++right;
					for (; right != current->cohorts.end(); ++right) {
						*left = *right;
						(*left)->local_number = lc;
						++lc;
						++left;
					}
					current->cohorts.resize(current->cohorts.size() - encs.size());
					for (auto eiter : encs) {
						eiter->type |= CT_ENCLOSED;
					}
					for (auto eiter2 : c->enclosed) {
						encs.push_back(eiter2);
					}
					c->enclosed = encs;
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
			u_fprintf(ux_stderr, " %S", tag.c_str());
		}
		u_fprintf(ux_stderr, "\n");
		u_fflush(ux_stderr);
		return;
	}

	if (trace_encl) {
		uint32_t hitpass = std::numeric_limits<uint32_t>::max() - pass;
		size_t nc = current->cohorts.size();
		for (size_t i = 0; i < nc; ++i) {
			Cohort* c = current->cohorts[i];
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
	if (!grammar->parentheses.empty() && current->has_enclosures) {
		size_t nc = current->cohorts.size();
		for (size_t i = 0; i < nc; ++i) {
			Cohort* c = current->cohorts[i];
			if (!c->enclosed.empty()) {
				current->cohorts.resize(current->cohorts.size() + c->enclosed.size(), 0);
				size_t ne = c->enclosed.size();
				for (size_t j = nc - 1; j > i; --j) {
					current->cohorts[j + ne] = current->cohorts[j];
					current->cohorts[j + ne]->local_number = j + ne;
				}
				for (size_t j = 0; j < ne; ++j) {
					current->cohorts[i + j + 1] = c->enclosed[j];
					current->cohorts[i + j + 1]->local_number = i + j + 1;
					current->cohorts[i + j + 1]->parent = current;
					current->cohorts[i + j + 1]->type &= ~CT_ENCLOSED;
				}
				par_left_tag = c->enclosed[0]->is_pleft;
				par_right_tag = c->enclosed[ne - 1]->is_pright;
				par_left_pos = i + 1;
				par_right_pos = i + ne;
				c->enclosed.clear();
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
}
}

// This helps the all_vislcg3.cpp profiling builds
#undef TRACE

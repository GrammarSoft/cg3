/*
* Copyright (C) 2007-2013, GrammarSoft ApS
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

namespace CG3 {

enum {
	RV_NOTHING = 1,
	RV_SOMETHING = 2,
	RV_DELIMITED = 4,
};

bool GrammarApplicator::updateRuleToCohorts(Cohort& c, const uint32_t& rsit) {
	// Check whether this rule is in the allowed rule list from cmdline flag --rule(s)
	if (!valid_rules.empty() && !valid_rules.contains(rsit)) {
		return false;
	}
	SingleWindow *current = c.parent;
	const Rule *r = grammar->rule_by_number[rsit];
	if (r->wordform && r->wordform != c.wordform) {
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
		const_foreach (uint32IntervalVector, (it->second), rsit, rsit_end) {
			if (updateRuleToCohorts(c, *rsit) && rules.contains(*rsit)) {
				intersects.insert(*rsit);
			}
		}
	}
	return (os != intersects.size());
}

void GrammarApplicator::indexSingleWindow(SingleWindow& current) {
	current.valid_rules.clear();
	current.rule_to_cohorts.clear();

	foreach (CohortVector, current.cohorts, iter, iter_end) {
		Cohort *c = *iter;
		foreach (uint32HashSet, c->possible_sets, psit, psit_end) {
			if (grammar->rules_by_set.find(*psit) == grammar->rules_by_set.end()) {
				continue;
			}
			const Grammar::rules_by_set_t::mapped_type& rules = grammar->rules_by_set.find(*psit)->second;
			const_foreach (Grammar::rules_by_set_t::mapped_type, rules, rsit, rsit_end) {
				updateRuleToCohorts(*c, *rsit);
			}
		}
	}
}

TagList GrammarApplicator::getTagList(const Set& theSet, bool unif_mode) const {
	TagList theTags;
	if (theSet.type & ST_SET_UNIFY) {
		const Set& pSet = *(grammar->getSet(theSet.sets[0]));
		const_foreach (uint32Vector, pSet.sets, iter, iter_end) {
			if (unif_sets.find(*iter) != unif_sets.end()) {
				TagList recursiveTags = getTagList(*(grammar->getSet(*iter)));
				theTags.splice(theTags.end(), recursiveTags);
			}
		}
	}
	else if (theSet.type & ST_TAG_UNIFY) {
		const_foreach (uint32Vector, theSet.sets, iter, iter_end) {
			TagList recursiveTags = getTagList(*(grammar->getSet(*iter)), true);
			theTags.splice(theTags.end(), recursiveTags);
		}
	}
	else if (!theSet.sets.empty()) {
		const_foreach (uint32Vector, theSet.sets, iter, iter_end) {
			TagList recursiveTags = getTagList(*(grammar->getSet(*iter)), unif_mode);
			theTags.splice(theTags.end(), recursiveTags);
		}
	}
	else if (unif_mode) {
		uint32HashMap::const_iterator iter = unif_tags.find(theSet.hash);
		if (iter != unif_tags.end()) {
			uint32_t ihash = iter->second;
			if (single_tags.find(ihash) != single_tags.end()) {
				theTags.push_back(single_tags.find(ihash)->second);
			}
			else if (grammar->tags.find(ihash) != grammar->tags.end()) {
				CompositeTag *tag = grammar->tags.find(ihash)->second;
				const_foreach (CompositeTag::tags_t, tag->tags, tter, tter_end) {
					theTags.push_back(*tter);
				}
			}
		}
	}
	else {
		const_foreach (AnyTagVector, theSet.tags_list, tter, tter_end) {
			if (tter->which == ANYTAG_TAG) {
				theTags.push_back(tter->getTag());
			}
			else {
				CompositeTag *tag = tter->getCompositeTag();
				const_foreach (CompositeTag::tags_t, tag->tags, tter, tter_end) {
					theTags.push_back(*tter);
				}
			}
		}
	}
	// Eliminate consecutive duplicates. Not all duplicates, since AddCohort and Append may have multiple readings with repeated tags
	for (TagList::iterator ot = theTags.begin() ; theTags.size() > 1 && ot != theTags.end() ; ++ot) {
		TagList::iterator it = ot;
		++it;
		for ( ; it != theTags.end() && std::distance(ot, it) == 1 ; ) {
			if (*ot == *it) {
				it = theTags.erase(it);
			}
			else {
				++it;
			}
		}
	}
	return theTags;
}

/**
 * Applies the passed rules to the passed SingleWindow.
 *
 * This function is called at least N*M times where N is number of sections in the grammar and M is the number of windows in the input.
 * Possibly many more times, since if a section changes the state of the window the section is run again.
 * Only when no further changes are caused at a level does it progress to next level.
 *
 * The loops in this function are increasingly explosive, despite efforts to contain them.
 * In the http://beta.visl.sdu.dk/cg3_performance.html test data, this function is called 1015 times.
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

	typedef stdext::hash_map<uint32_t,Reading*> readings_plain_t;
	readings_plain_t readings_plain;

	// ToDo: Now that numbering is used, can't this be made a normal max? Hm, maybe not since --sections can still force another order...but if we're smart, then we re-enumerate rules based on --sections
	uint32IntervalVector intersects = current.valid_rules.intersect(rules);

	if (debug_level > 1) {
		std::cerr << "DEBUG: Trying window " << current.number << std::endl;
	}

	const_foreach (uint32IntervalVector, intersects, iter_rules, iter_rules_end) {
		uint32_t j = (*iter_rules);

		// Check whether this rule is in the allowed rule list from cmdline flag --rule(s)
		if (!valid_rules.empty() && !valid_rules.contains(j)) {
			continue;
		}

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

		const Set& set = *(grammar->getSet(rule.target));
		grammar->lines = rule.line;

		uint32ToCohortsMap::iterator csit = current.rule_to_cohorts.find(rule.number);
		if (csit == current.rule_to_cohorts.end()) {
			continue;
		}
		CohortSet *cohortset = &csit->second;
		if (debug_level > 1) {
			std::cerr << "DEBUG: " << cohortset->size() << "/" << current.cohorts.size() << " = " << double(cohortset->size())/double(current.cohorts.size()) << std::endl;
		}
		for (CohortSet::const_iterator rocit = cohortset->begin() ; rocit != cohortset->end() ; ) {
			Cohort *cohort = *rocit;
			++rocit;

			if (debug_level > 1) {
				std::cerr << "DEBUG: Trying cohort " << cohort->local_number << std::endl;
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
			if (rule.sub_reading == 0 && cohort->possible_sets.find(rule.target) == cohort->possible_sets.end()) {
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
			if (type == K_DELIMIT && c == current.cohorts.size()-1) {
				continue;
			}
			// If the rule has a wordform and it is not this one, skip it.
			// ToDo: Is this even used still? updateRuleToCohorts() should handle this now.
			if (rule.wordform && rule.wordform != cohort->wordform) {
				++rule.num_fail;
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
			uint32_t ih = hash_sdbm_uint32_t(rule.number, cohort->global_number);
			if (index_matches(index_ruleCohort_no, ih)) {
				continue;
			}
			index_ruleCohort_no.insert(ih);

			size_t num_active = 0;
			size_t num_iff = 0;

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
			// Varstring capture groups exist on a per-cohort basis, since we may need them for mapping later.
			if (!regexgrps.empty()) {
				regexgrps.clear();
			}

			// This loop figures out which readings, if any, that are valid targets for the current rule
			// Criteria for valid is that the reading must match both target and all contextual tests
			foreach (ReadingList, cohort->readings, rter1, rter1_end) {
				Reading *reading = get_sub_reading(*rter1, rule.sub_reading);
				if (!reading) {
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

				// Check if any previous reading of this cohort had the same plain signature, and if so just copy their results
				// This cache is cleared on a per-cohort basis
				if (!(set.type & (ST_MAPPING|ST_CHILD_UNIFY)) && !readings_plain.empty()) {
					readings_plain_t::const_iterator rpit = readings_plain.find(reading->hash_plain);
					if (rpit != readings_plain.end()) {
						reading->matched_target = rpit->second->matched_target;
						reading->matched_tests = rpit->second->matched_tests;
						if (reading->matched_tests) {
							++num_active;
						}
						continue;
					}
				}

				// Unification is done on a per-reading basis, so clear all unification state.
				// ToDo: Doesn't this mess up the new sets-for-mapping? Yes it does, but due a missing did_test = true it still works...
				// ToDo: Need to tie unification data to the readings directly before fixing the did_test bug.
				unif_last_wordform = 0;
				unif_last_baseform = 0;
				unif_last_textual = 0;
				if (!unif_tags.empty()) {
					unif_tags.clear();
				}
				unif_sets_firstrun = true;
				if (!unif_sets.empty()) {
					unif_sets.clear();
				}

				target = 0;
				mark = cohort;
				// Actually check if the reading is a valid target. First check if rule target matches...
				if (rule.target && doesSetMatchReading(*reading, rule.target, (set.type & (ST_CHILD_UNIFY|ST_SPECIAL)) != 0)) {
					target = cohort;
					reading->matched_target = true;
					matched_target = true;
					bool good = true;
					// If we didn't already run the contextual tests, run them now.
					// This only needs to be done once per cohort as no current functionality exists to refer back to the exact reading.
					if (!did_test) {
						// Contextual tests are stored as a linked list, so looping through them looks a bit different.
						foreach (ContextList, rule.tests, it, it_end) {
							ContextualTest *test = *it;
							if (rule.flags & RF_RESETX || !(rule.flags & RF_REMEMBERX)) {
								mark = cohort;
							}
							// Keeps track of where we have been, to prevent infinite recursion in trees with loops
							dep_deep_seen.clear();
							// Reset the counters for which types of CohortIterator we have in play
							std::fill(ci_depths.begin(), ci_depths.end(), 0);
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
									if (it != rule.tests.begin() && !(rule.flags & (RF_REMEMBERX|RF_KEEPORDER))) {
										rule.tests.erase(it);
										rule.tests.push_front(test);
									}
									break;
								}
							}
						}
					}
					else if (did_test) {
						good = test_good;
					}
					if (good) {
						// We've found a match, so Iff should be treated as Select instead of Remove
						if (rule.type == K_IFF) {
							type = K_SELECT;
						}
						reading->matched_tests = true;
						++num_active;
						++rule.num_match;
					}
					++num_iff;
				}
				else {
					++rule.num_fail;
				}
				readings_plain.insert(std::make_pair(reading->hash_plain,reading));
			}

			// If none of the readings were valid targets, remove this cohort from the rule's possible cohorts.
			if (num_active == 0 && (num_iff == 0 || rule.type != K_IFF)) {
				if (!matched_target) {
					--rocit; // We have already incremented rocit earlier, so take one step back...
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
			ReadingList removed;
			ReadingList selected;

			// Remember the current state so we can compare later to see if anything has changed
			const size_t state_num_readings = cohort->readings.size();
			const size_t state_num_removed = cohort->deleted.size();
			const size_t state_num_delayed = cohort->delayed.size();
			bool readings_changed = false;

			// This loop acts on the result of the previous loop; letting the rules do their thing on the valid readings.
			for (size_t i=0 ; i<cohort->readings.size() ; ++i) {
				Reading *tr = get_sub_reading(cohort->readings[i], rule.sub_reading);
				if (!tr) {
					tr = cohort->readings[i];
					tr->matched_target = false;
				}

				Reading& reading = *tr;
				Reading& reading_head = *cohort->readings[i];
				bool good = reading.matched_tests;
				const uint32_t state_hash = reading.hash;

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
						reading.hit_by.push_back(rule.number);
					}
					continue;
				}

				// Select is also special as it will remove non-matching readings
				if (type == K_SELECT) {
					if (good) {
						selected.push_back(&reading_head);
						index_ruleCohort_no.clear();
						reading.hit_by.push_back(rule.number);
					}
					else {
						removed.push_back(&reading_head);
						index_ruleCohort_no.clear();
						reading.hit_by.push_back(rule.number);
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
						if ((rule.flags & RF_UNMAPLAST) && removed.size() == cohort->readings.size()-1) {
							if (unmapReading(reading, rule.number)) {
								readings_changed = true;
							}
						}
						else {
							removed.push_back(&reading_head);
							reading.hit_by.push_back(rule.number);
						}
						index_ruleCohort_no.clear();
						if (debug_level > 0) {
							std::cerr << "DEBUG: Rule " << rule.line << " hit cohort " << cohort->local_number << std::endl;
						}
					}
					else if (type == K_JUMP) {
						reading.hit_by.push_back(rule.number);
						const Tag *to = getTagList(*rule.maplist).front();
						uint32HashMap::const_iterator it = grammar->anchors.find(to->hash);
						if (it == grammar->anchors.end()) {
							u_fprintf(ux_stderr, "Warning: JUMP on line %u could not find anchor '%S'.\n", rule.line, to->tag.c_str());
						}
						else {
							iter_rules = intersects.lower_bound(it->second);
							--iter_rules;
						}
						break;
					}
					else if (type == K_REMVARIABLE) {
						reading.hit_by.push_back(rule.number);
						const TagList names = getTagList(*rule.maplist);
						const_foreach (TagList, names, tter, tter_end) {
							const Tag *tag = *tter;
							variables.erase(tag->hash);
							//u_fprintf(ux_stderr, "Info: RemVariable fired for %S.\n", tag->tag.c_str());
						}
						break;
					}
					else if (type == K_SETVARIABLE) {
						reading.hit_by.push_back(rule.number);
						const TagList names = getTagList(*rule.maplist);
						const TagList values = getTagList(*rule.sublist);
						variables[names.front()->hash] = values.front()->hash;
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
						if (type == K_EXTERNAL_ONCE && !current.hit_external.insert(rule.line)) {
							break;
						}

						externals_t::iterator ei = externals.find(rule.varname);
						if (ei == externals.end()) {
							Tag *ext = single_tags.find(rule.varname)->second;
							UErrorCode err = U_ZERO_ERROR;
							u_strToUTF8(&cbuffers[0][0], CG3_BUFFER_SIZE-1, 0, ext->tag.c_str(), ext->tag.length(), &err);

							exec_stream_t *es = 0;
							try {
								es = new exec_stream_t;
								es->set_binary_mode(exec_stream_t::s_in);
								es->set_binary_mode(exec_stream_t::s_out);
								es->set_wait_timeout(exec_stream_t::s_in, 10000);
								es->set_wait_timeout(exec_stream_t::s_out, 10000);
								es->start(&cbuffers[0][0], "");
								writeRaw(es->in(), CG3_EXTERNAL_PROTOCOL);
							}
							catch (std::exception& e) {
								u_fprintf(ux_stderr, "Error: External on line %u resulted in error: %s\n", rule.line, e.what());
								CG3Quit(1);
							}
							externals[rule.varname] = es;
							ei = externals.find(rule.varname);
						}

						pipeOutSingleWindow(current, ei->second->in());
						pipeInSingleWindow(current, ei->second->out());

						indexSingleWindow(current);
						readings_changed = true;
						index_ruleCohort_no.clear();
						intersects = current.valid_rules.intersect(rules);
						iter_rules = intersects.find(rule.number);
						iter_rules_end = intersects.end();
						cohortset = &current.rule_to_cohorts.find(rule.number)->second;
						rocit = cohortset->find(cohort);
						++rocit;
						break;
					}
					else if (type == K_REMCOHORT) {
						foreach (ReadingList, cohort->readings, iter, iter_end) {
							(*iter)->hit_by.push_back(rule.number);
							(*iter)->deleted = true;
						}
						cohort->type |= CT_REMOVED;
						cohort->prev->removed.push_back(cohort);
						cohort->detach();
						cohort->parent = 0;
						current.cohorts.erase(current.cohorts.begin()+cohort->local_number);
						foreach (CohortVector, current.cohorts, iter, iter_end) {
							(*iter)->local_number = std::distance(current.cohorts.begin(), iter);
						}
						gWindow->rebuildCohortLinks();
						readings_changed = true;
						break;
					}
					else if (type == K_ADDCOHORT_AFTER || type == K_ADDCOHORT_BEFORE) {
						reading.hit_by.push_back(rule.number);
						index_ruleCohort_no.clear();

						Cohort *cCohort = new Cohort(&current);
						cCohort->global_number = gWindow->cohort_counter++;

						Tag *wf = 0;
						std::vector<TagList> readings;
						const TagList theTags = getTagList(*rule.maplist);
						const_foreach (TagList, theTags, tter, tter_end) {
							if ((*tter)->type & T_WORDFORM) {
								cCohort->wordform = (*tter)->hash;
								wf = *tter;
								continue;
							}
							assert(wf && "There must be a wordform before any other tags in ADDCOHORT.");
							if ((*tter)->type & T_BASEFORM) {
								readings.resize(readings.size()+1);
								readings.back().push_back(wf);
							}
							readings.back().push_back(*tter);
						}

						foreach (std::vector<TagList>, readings, rit, rit_end) {
							Reading *cReading = new Reading(cCohort);
							++numReadings;
							insert_if_exists(cReading->parent->possible_sets, grammar->sets_any);
							cReading->hit_by.push_back(rule.number);
							cReading->noprint = false;
							TagList mappings;
							const_foreach (TagList, *rit, tter, tter_end) {
								uint32_t hash = (*tter)->hash;
								if ((*tter)->type & T_MAPPING || (*tter)->tag[0] == grammar->mapping_prefix) {
									mappings.push_back(*tter);
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
						}

						if (type == K_ADDCOHORT_BEFORE) {
							current.cohorts.insert(current.cohorts.begin() + cohort->local_number, cCohort);
						}
						else {
							current.cohorts.insert(current.cohorts.begin() + cohort->local_number + 1, cCohort);
						}

						foreach (CohortVector, current.cohorts, iter, iter_end) {
							(*iter)->local_number = std::distance(current.cohorts.begin(), iter);
						}
						gWindow->rebuildCohortLinks();
						indexSingleWindow(current);
						readings_changed = true;

						cohortset = &current.rule_to_cohorts.find(rule.number)->second;
						rocit = cohortset->find(cohort);
						++rocit;
						break;
					}
					else if (rule.type == K_ADD || rule.type == K_MAP) {
						index_ruleCohort_no.clear();
						reading.hit_by.push_back(rule.number);
						reading.noprint = false;
						TagList mappings;
						const TagList theTags = getTagList(*rule.maplist);
						const_foreach (TagList, theTags, tter, tter_end) {
							uint32_t hash = (*tter)->hash;
							if ((*tter)->type & T_MAPPING || (*tter)->tag[0] == grammar->mapping_prefix) {
								mappings.push_back(*tter);
							}
							else {
								hash = addTagToReading(reading, hash);
							}
							if (updateValidRules(rules, intersects, hash, reading)) {
								iter_rules = intersects.find(rule.number);
								iter_rules_end = intersects.end();
							}
						}
						if (!mappings.empty()) {
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
						reading.hit_by.push_back(rule.number);
						reading.noprint = false;
						reading.tags_list.clear();
						reading.tags_list.push_back(reading.wordform);
						reading.tags_list.push_back(reading.baseform);
						reflowReading(reading);
						TagList mappings;
						TagList theTags = getTagList(*rule.maplist);
						const_foreach (TagList, theTags, tter, tter_end) {
							uint32_t hash = (*tter)->hash;
							if ((*tter)->type & T_MAPPING || (*tter)->tag[0] == grammar->mapping_prefix) {
								mappings.push_back(*tter);
							}
							else {
								hash = addTagToReading(reading, hash);
							}
							if (updateValidRules(rules, intersects, hash, reading)) {
								iter_rules = intersects.find(rule.number);
								iter_rules_end = intersects.end();
							}
						}
						if (!mappings.empty()) {
							splitMappings(mappings, *cohort, reading, true);
						}
						if (reading.hash != state_hash) {
							readings_changed = true;
						}
					}
					else if (rule.type == K_SUBSTITUTE) {
						// ToDo: Check whether this substitution will do nothing at all to the end result
						// ToDo: Not actually...instead, test whether any reading in the cohort already is the end result

						size_t tpos = std::numeric_limits<size_t>::max();
						size_t tagb = reading.tags_list.size();
						TagList theTags = getTagList(*rule.sublist);

						// Modify the list of tags to remove to be the actual list of tags present, including matching regex and icase tags
						for (TagList::iterator it = theTags.begin() ; it != theTags.end() ; ) {
							if (reading.tags.find((*it)->hash) == reading.tags.end()) {
								const Tag* tt = *it;
								it = theTags.erase(it);
								if (tt->type & T_SPECIAL) {
									uint32_t stag = doesTagMatchReading(reading, *tt, false, true);
									if (stag) {
										theTags.insert(it, single_tags.find(stag)->second);
									}
								}
								continue;
							}
							++it;
						}

						// Perform the tag removal, remembering the position of the final removed tag for use as insertion spot
						const_foreach (TagList, theTags, tter, tter_end) {
							if (tpos == std::numeric_limits<size_t>::max()) {
								foreach (Reading::tags_list_t, reading.tags_list, tfind, tfind_end) {
									if (*tfind == (*tter)->hash) {
										tpos = std::distance(reading.tags_list.begin(), tfind);
										--tpos;
										break;
									}
								}
							}
							erase(reading.tags_list, (*tter)->hash);
							reading.tags.erase((*tter)->hash);
							if (reading.baseform == (*tter)->hash) {
								reading.baseform = 0;
							}
						}

						// Should Substitute really do nothing if no tags were removed? 2013-10-21, Eckhard says this is expected behavior.
						if (tagb != reading.tags_list.size()) {
							uint32_t wf = 0;
							index_ruleCohort_no.clear();
							reading.hit_by.push_back(rule.number);
							reading.noprint = false;
							if (tpos == std::numeric_limits<size_t>::max()) {
								tpos = reading.tags_list.size() - 1;
							}
							++tpos;
							TagList mappings;
							TagList theTags = getTagList(*rule.maplist);
							const_foreach (TagList, theTags, tter, tter_end) {
								if ((*tter)->hash == grammar->tag_any) {
									break;
								}
								if (reading.tags.find((*tter)->hash) != reading.tags.end()) {
									continue;
								}
								if ((*tter)->type & T_MAPPING || (*tter)->tag[0] == grammar->mapping_prefix) {
									mappings.push_back(*tter);
								}
								else {
									if ((*tter)->type & T_WORDFORM) {
										wf = (*tter)->hash;
									}
									reading.tags_list.insert(reading.tags_list.begin()+tpos, (*tter)->hash);
									++tpos;
								}
								if (updateValidRules(rules, intersects, (*tter)->hash, reading)) {
									iter_rules = intersects.find(rule.number);
									iter_rules_end = intersects.end();
								}
							}
							reflowReading(reading);
							if (!mappings.empty()) {
								splitMappings(mappings, *cohort, reading, true);
							}
							if (wf && wf != reading.parent->wordform) {
								boost_foreach (Reading *r, reading.parent->readings) {
									delTagFromReading(*r, reading.parent->wordform);
									addTagToReading(*r, wf);
								}
								boost_foreach (Reading *r, reading.parent->deleted) {
									delTagFromReading(*r, reading.parent->wordform);
									addTagToReading(*r, wf);
								}
								boost_foreach (Reading *r, reading.parent->delayed) {
									delTagFromReading(*r, reading.parent->wordform);
									addTagToReading(*r, wf);
								}
								reading.parent->wordform = wf;
							}
						}
						if (reading.hash != state_hash) {
							readings_changed = true;
						}
					}
					else if (rule.type == K_APPEND) {
						index_ruleCohort_no.clear();

						Tag *bf = 0;
						std::vector<TagList> readings;
						const TagList theTags = getTagList(*rule.maplist);
						const_foreach (TagList, theTags, tter, tter_end) {
							if ((*tter)->type & T_BASEFORM) {
								bf = *tter;
								readings.resize(readings.size()+1);
							}
							assert(bf && "There must be a baseform before any other tags in APPEND.");
							readings.back().push_back(*tter);
						}

						foreach (std::vector<TagList>, readings, rit, rit_end) {
							Reading *cReading = new Reading(cohort);
							++numReadings;
							insert_if_exists(cReading->parent->possible_sets, grammar->sets_any);
							addTagToReading(*cReading, cohort->wordform);
							cReading->hit_by.push_back(rule.number);
							cReading->noprint = false;
							TagList mappings;
							const_foreach (TagList, *rit, tter, tter_end) {
								uint32_t hash = (*tter)->hash;
								if ((*tter)->type & T_MAPPING || (*tter)->tag[0] == grammar->mapping_prefix) {
									mappings.push_back(*tter);
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
								splitMappings(mappings, *cohort, *cReading);
							}
							cohort->appendReading(cReading);
						}

						if (cohort->readings.size() > 1) {
							foreach (ReadingList, cohort->readings, rit, rit_end) {
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
						Reading *cReading = cohort->allocateAppendReading();
						++numReadings;
						index_ruleCohort_no.clear();
						cReading->hit_by.push_back(rule.number);
						cReading->noprint = false;
						const_foreach (Reading::tags_list_t, reading.tags_list, iter, iter_end) {
							addTagToReading(*cReading, *iter);
						}
						TagList mappings;
						TagList theTags = getTagList(*rule.maplist);
						const_foreach (TagList, theTags, tter, tter_end) {
							uint32_t hash = (*tter)->hash;
							if ((*tter)->type & T_MAPPING || (*tter)->tag[0] == grammar->mapping_prefix) {
								mappings.push_back(*tter);
							}
							else {
								hash = addTagToReading(*cReading, hash);
							}
							if (updateValidRules(rules, intersects, hash, reading)) {
								iter_rules = intersects.find(rule.number);
								iter_rules_end = intersects.end();
							}
						}
						if (!mappings.empty()) {
							splitMappings(mappings, *cohort, *cReading, true);
						}
						readings_changed = true;
					}
					else if (type == K_SETPARENT || type == K_SETCHILD) {
						int32_t orgoffset = rule.dep_target->offset;
						uint32Set seen_targets;

						seen_barrier = false;
						bool attached = false;
						Cohort *target = cohort;
						while (!attached) {
							Cohort *attach = 0;
							seen_targets.insert(target->global_number);
							dep_deep_seen.clear();
							attach_to = 0;
							if (runContextualTest(target->parent, target->local_number, rule.dep_target, &attach) && attach) {
								if (attach_to) {
									attach = attach_to;
								}
								bool good = true;
								foreach (ContextList, rule.dep_tests, it, it_end) {
									mark = attach;
									dep_deep_seen.clear();
									test_good = (runContextualTest(attach->parent, attach->local_number, *it) != 0);
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
										reading.hit_by.push_back(rule.number);
										reading.noprint = false;
										has_dep = true;
										readings_changed = true;
										break;
									}
								}
								if (seen_barrier || (rule.flags & RF_NEAREST)) {
									break;
								}
								if (seen_targets.find(attach->global_number) != seen_targets.end()) {
									// We've found a cohort we have seen before...
									// We assume running the test again would result in the same, so don't bother.
									break;
								}
								if (!attached) {
									// Did not successfully attach due to loop restrictions; look onwards from here
									target = attach;
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
						Cohort *attach = 0;
						dep_deep_seen.clear();
						attach_to = 0;
						if (runContextualTest(&current, c, rule.dep_target, &attach) && attach && cohort->parent == attach->parent) {
							if (attach_to) {
								attach = attach_to;
							}
							bool good = true;
							foreach (ContextList, rule.dep_tests, it, it_end) {
								mark = attach;
								dep_deep_seen.clear();
								test_good = (runContextualTest(attach->parent, attach->local_number, *it) != 0);
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
								foreach (ReadingList, cohort->readings, iter, iter_end) {
									(*iter)->hit_by.push_back(rule.number);
								}
								foreach (ReadingList, attach->readings, iter, iter_end) {
									(*iter)->hit_by.push_back(rule.number);
								}
							}
							else {
								CohortVector cohorts;
								if (rule.childset1) {
									for (CohortVector::iterator iter = current.cohorts.begin() ; iter != current.cohorts.end() ; ) {
										if (isChildOf(*iter, cohort) && doesSetMatchCohortNormal(**iter, rule.childset1)) {
											cohorts.push_back(*iter);
											iter = current.cohorts.erase(iter);
										}
										else {
											++iter;
										}
									}
								}
								else {
									cohorts.push_back(cohort);
									current.cohorts.erase(current.cohorts.begin()+cohort->local_number);
								}

								foreach (CohortVector, current.cohorts, iter, iter_end) {
									(*iter)->local_number = std::distance(current.cohorts.begin(), iter);
								}

								CohortVector edges;
								if (rule.childset2) {
									foreach (CohortVector, current.cohorts, iter, iter_end) {
										if (isChildOf(*iter, attach) && doesSetMatchCohortNormal(**iter, rule.childset2)) {
											edges.push_back(*iter);
										}
									}
								}
								else {
									edges.push_back(attach);
								}
								uint32_t spot = 0;
								if (type == K_MOVE_BEFORE) {
									spot = edges.front()->local_number;
									if (spot == 0) {
										spot = 1;
									}
								}
								else if (type == K_MOVE_AFTER) {
									spot = edges.back()->local_number+1;
								}

								while (!cohorts.empty()) {
									foreach (ReadingList, cohorts.back()->readings, iter, iter_end) {
										(*iter)->hit_by.push_back(rule.number);
									}
									current.cohorts.insert(current.cohorts.begin()+spot, cohorts.back());
									cohorts.pop_back();
								}
							}
							foreach (CohortVector, current.cohorts, iter, iter_end) {
								(*iter)->local_number = std::distance(current.cohorts.begin(), iter);
							}
							gWindow->rebuildCohortLinks();
							readings_changed = true;
							break;
						}
					}
					else if (type == K_ADDRELATION || type == K_SETRELATION || type == K_REMRELATION) {
						Cohort *attach = 0;
						dep_deep_seen.clear();
						attach_to = 0;
						// ToDo: Maybe allow SetRelation family to scan on after failed tests?
						if (runContextualTest(&current, c, rule.dep_target, &attach) && attach) {
							if (attach_to) {
								attach = attach_to;
							}
							bool good = true;
							foreach (ContextList, rule.dep_tests, it, it_end) {
								mark = attach;
								dep_deep_seen.clear();
								test_good = (runContextualTest(attach->parent, attach->local_number, *it) != 0);
								if (!test_good) {
									good = test_good;
									break;
								}
							}
							if (good) {
								swapper<Cohort*> sw((rule.flags & RF_REVERSE) != 0, attach, cohort);
								bool rel_did_anything = false;
								TagList theTags = getTagList(*rule.maplist);
								const_foreach (TagList, theTags, tter, tter_end) {
									if (type == K_ADDRELATION) {
										attach->type |= CT_RELATED;
										cohort->type |= CT_RELATED;
										rel_did_anything |= cohort->addRelation((*tter)->hash, attach->global_number);
									}
									else if (type == K_SETRELATION) {
										attach->type |= CT_RELATED;
										cohort->type |= CT_RELATED;
										rel_did_anything |= cohort->setRelation((*tter)->hash, attach->global_number);
									}
									else {
										rel_did_anything |= cohort->remRelation((*tter)->hash, attach->global_number);
									}
								}
								if (rel_did_anything) {
									index_ruleCohort_no.clear();
									reading.hit_by.push_back(rule.number);
									reading.noprint = false;
									readings_changed = true;
								}
							}
						}
						break;
					}
					else if (type == K_ADDRELATIONS || type == K_SETRELATIONS || type == K_REMRELATIONS) {
						Cohort *attach = 0;
						dep_deep_seen.clear();
						attach_to = 0;
						if (runContextualTest(&current, c, rule.dep_target, &attach) && attach) {
							if (attach_to) {
								attach = attach_to;
							}
							bool good = true;
							foreach (ContextList, rule.dep_tests, it, it_end) {
								mark = attach;
								dep_deep_seen.clear();
								test_good = (runContextualTest(attach->parent, attach->local_number, *it) != 0);
								if (!test_good) {
									good = test_good;
									break;
								}
							}
							if (good) {
								swapper<Cohort*> sw((rule.flags & RF_REVERSE) != 0, attach, cohort);
								bool rel_did_anything = false;
								TagList sublist = getTagList(*rule.sublist);
								TagList maplist = getTagList(*rule.maplist);
								const_foreach (TagList, maplist, tter, tter_end) {
									if (type == K_ADDRELATIONS) {
										cohort->type |= CT_RELATED;
										rel_did_anything |= cohort->addRelation((*tter)->hash, attach->global_number);
									}
									else if (type == K_SETRELATIONS) {
										cohort->type |= CT_RELATED;
										rel_did_anything |= cohort->setRelation((*tter)->hash, attach->global_number);
									}
									else {
										rel_did_anything |= cohort->remRelation((*tter)->hash, attach->global_number);
									}
								}
								const_foreach (TagList, sublist, tter, tter_end) {
									if (type == K_ADDRELATIONS) {
										attach->type |= CT_RELATED;
										rel_did_anything |= attach->addRelation((*tter)->hash, cohort->global_number);
									}
									else if (type == K_SETRELATIONS) {
										attach->type |= CT_RELATED;
										rel_did_anything |= attach->setRelation((*tter)->hash, cohort->global_number);
									}
									else {
										rel_did_anything |= attach->remRelation((*tter)->hash, cohort->global_number);
									}
								}
								if (rel_did_anything) {
									index_ruleCohort_no.clear();
									reading.hit_by.push_back(rule.number);
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
					for (size_t i=0 ; i<oz ; ++i) {
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
			if (state_num_readings != cohort->readings.size()
				|| state_num_removed != cohort->deleted.size()
				|| state_num_delayed != cohort->delayed.size()
				|| readings_changed) {
				if (!(rule.flags & RF_NOITERATE) && section_max_count != 1) {
					section_did_something = true;
				}
				cohort->type &= ~CT_NUM_CURRENT;
			}

			if (delimited) {
				break;
			}
		}

		if (statistics) {
			ticks tmp = getticks();
			rule.total_time += elapsed(tmp, tstamp);
		}

		if (delimited) {
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
		if (rv & RV_DELIMITED) {
			return rv;
		}
	}

	if (!grammar->rules.empty() && !no_sections) {
		std::map<uint32_t,uint32_t> counter;
		// Caveat: This may look as if it is not recursing previous sections, but those rules are preprocessed into the successive sections so they are actually run.
		RSType::iterator iter = runsections.begin();
		RSType::iterator iter_end = runsections.end();
		for (; iter != iter_end ;) {
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
			if (rv & RV_DELIMITED) {
				return rv;
			}
			if (!(rv & RV_SOMETHING)) {
				++iter;
			}
		}
	}

	if (!grammar->after_sections.empty() && !no_after_sections) {
		uint32_t rv = runRulesOnSingleWindow(current, runsections[-2]);
		if (rv & RV_DELIMITED) {
			return rv;
		}
	}

	return 0;
}

void GrammarApplicator::runGrammarOnWindow() {
	SingleWindow *current = gWindow->current;
	did_final_enclosure = false;

	const_foreach (uint32HashMap, current->variables_set, vit, vit_end) {
		variables[vit->first] = vit->second;
	}
	const_foreach (uint32HashSet, current->variables_rem, vit, vit_end) {
		variables.erase(*vit);
	}

	if (has_dep) {
		reflowDependencyWindow();
		gWindow->dep_map.clear();
		gWindow->dep_window.clear();
		dep_highest_seen = 0;
		if (!input_eof && !gWindow->next.empty() && gWindow->next.back()->cohorts.size() > 1) {
			foreach (CohortVector, gWindow->next.back()->cohorts, iter, iter_end) {
				Cohort *cohort = *iter;
				gWindow->dep_window[cohort->global_number] = cohort;
			}
		}
	}
	if (has_relations) {
		reflowRelationWindow();
	}

	if (!grammar->parentheses.empty()) {
		label_scanParentheses:
		reverse_foreach (CohortVector, current->cohorts, iter, iter_end) {
			Cohort *c = *iter;
			if (c->is_pleft == 0) {
				continue;
			}
			uint32Map::const_iterator p = grammar->parentheses.find(c->is_pleft);
			if (p != grammar->parentheses.end()) {
				CohortVector::iterator right = iter.base();
				--right;
				--right;
				c = *right;
				++right;
				bool found = false;
				CohortVector encs;
				for (; right != current->cohorts.end() ; ++right) {
					Cohort *s = *right;
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
					for (; right != current->cohorts.end() ; ++right) {
						*left = *right;
						(*left)->local_number = lc;
						++lc;
						++left;
					}
					current->cohorts.resize(current->cohorts.size() - encs.size());
					foreach (CohortVector, encs, eiter, eiter_end) {
						(*eiter)->type |= CT_ENCLOSED;
					}
					foreach (CohortVector, c->enclosed, eiter2, eiter2_end) {
						encs.push_back(*eiter2);
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
		u_fprintf(ux_stderr, "Warning: Endless loop detected before input line %u - will try to break it.\n", numLines);
		return;
	}

	if (trace_encl) {
		uint32_t hitpass = std::numeric_limits<uint32_t>::max() - pass;
		size_t nc = current->cohorts.size();
		for (size_t i=0 ; i<nc ; ++i) {
			Cohort *c = current->cohorts[i];
			foreach (ReadingList, c->readings, rit, rit_end) {
				(*rit)->hit_by.push_back(hitpass);
			}
		}
	}

	uint32_t rv = runGrammarOnSingleWindow(*current);
	if (rv & RV_DELIMITED) {
		goto label_runGrammarOnWindow_begin;
	}

	if (!grammar->parentheses.empty() && current->has_enclosures) {
		bool found = false;
		size_t nc = current->cohorts.size();
		for (size_t i=0 ; i<nc ; ++i) {
			Cohort *c = current->cohorts[i];
			if (!c->enclosed.empty()) {
				current->cohorts.resize(current->cohorts.size() + c->enclosed.size(), 0);
				size_t ne = c->enclosed.size();
				for (size_t j=nc-1 ; j>i ; --j) {
					current->cohorts[j+ne] = current->cohorts[j];
					current->cohorts[j+ne]->local_number = j+ne;
				}
				for (size_t j=0 ; j<ne ; ++j) {
					current->cohorts[i+j+1] = c->enclosed[j];
					current->cohorts[i+j+1]->local_number = i+j+1;
					current->cohorts[i+j+1]->parent = current;
					current->cohorts[i+j+1]->type &= ~CT_ENCLOSED;
				}
				par_left_tag = c->enclosed[0]->is_pleft;
				par_right_tag = c->enclosed[ne-1]->is_pright;
				par_left_pos = i+1;
				par_right_pos = i+ne;
				c->enclosed.clear();
				found = true;
				goto label_runGrammarOnWindow_begin;
			}
		}
		if (!found && !did_final_enclosure) {
			par_left_tag = 0;
			par_right_tag = 0;
			par_left_pos = 0;
			par_right_pos = 0;
			did_final_enclosure = true;
			goto label_runGrammarOnWindow_begin;
		}
	}
}

}

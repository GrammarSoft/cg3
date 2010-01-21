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

#include "GrammarApplicator.h"
#include "Strings.h"
#include "Tag.h"
#include "Grammar.h"
#include "Window.h"
#include "SingleWindow.h"
#include "Reading.h"
#include "ContextualTest.h"

namespace CG3 {

void GrammarApplicator::updateRuleToCohorts(Cohort& c, const uint32_t& rsit) {
	SingleWindow *current = c.parent;
	const Rule *r = grammar->rule_by_line.find(rsit)->second;
	if (r->wordform && r->wordform != c.wordform) {
		return;
	}
	CohortSet& s = current->rule_to_cohorts[r];
	s.insert(&c);
	current->valid_rules.insert(r->line);
}

void GrammarApplicator::updateValidRules(const uint32Set& rules, uint32Set &intersects, const uint32_t& hash, Reading &reading) {
	uint32HashSetuint32HashMap::const_iterator it = grammar->rules_by_tag.find(hash);
	if (it != grammar->rules_by_tag.end()) {
		SingleWindow &current = *(reading.parent->parent);
		Cohort &c = *(reading.parent);
		const_foreach (uint32HashSet, (*(it->second)), rsit, rsit_end) {
			updateRuleToCohorts(c, *rsit);
		}

		std::set_intersection(rules.begin(), rules.end(),
			current.valid_rules.begin(), current.valid_rules.end(),
			std::inserter(intersects, intersects.begin()));
	}
}

void GrammarApplicator::indexSingleWindow(SingleWindow &current) {
	current.valid_rules.clear();

	foreach (CohortVector, current.cohorts, iter, iter_end) {
		Cohort *c = *iter;
		foreach (uint32HashSet, c->possible_sets, psit, psit_end) {
			if (grammar->rules_by_set.find(*psit) == grammar->rules_by_set.end()) {
				continue;
			}
			const uint32Set *rules = grammar->rules_by_set.find(*psit)->second;
			const_foreach (uint32Set, (*rules), rsit, rsir_end) {
				updateRuleToCohorts(*c, *rsit);
			}
		}
	}
}

uint32_t GrammarApplicator::runRulesOnWindow(SingleWindow &current, uint32Set &rules) {
	uint32_t retval = RV_NOTHING;
	bool section_did_good = false;
	bool delimited = false;

	uint32Set intersects;
	std::set_intersection(rules.begin(), rules.end(),
		current.valid_rules.begin(), current.valid_rules.end(),
		std::inserter(intersects, intersects.begin()));

	foreach (uint32Set, intersects, iter_rules, iter_rules_end) {
		uint32_t j = (*iter_rules);
		const Rule &rule = *(grammar->rule_by_line.find(j)->second);

		ticks tstamp(gtimer);
		KEYWORDS type = rule.type;

		if (!apply_mappings && (rule.type == K_MAP || rule.type == K_ADD || rule.type == K_REPLACE)) {
			continue;
		}
		if (!apply_corrections && (rule.type == K_SUBSTITUTE || rule.type == K_APPEND)) {
			continue;
		}
		if (has_enclosures) {
			if (rule.flags & RF_ENCL_FINAL && !did_final_enclosure) {
				continue;
			}
			else if (did_final_enclosure && !(rule.flags & RF_ENCL_FINAL)) {
				continue;
			}
		}
		if (statistics) {
			tstamp = getticks();
		}

		const Set &set = *(grammar->sets_by_contents.find(rule.target)->second);

		// ToDo: Update list of in/valid rules upon MAP, ADD, REPLACE, APPEND, SUBSTITUTE; add tags + always add tag_any
		// ToDo: Make better use of rules_by_tag

		//for (size_t c=1 ; c < current.cohorts.size() ; c++) {
		foreach (CohortSet, current.rule_to_cohorts.find(&rule)->second, rocit, rocit_end) {
			Cohort *cohort = *rocit;
			if (cohort->local_number == 0) {
				continue;
			}

			uint32_t c = cohort->local_number;
			if (cohort->is_enclosed || cohort->parent != &current) {
				continue;
			}
			if (cohort->readings.empty()) {
				continue;
			}
			if (cohort->possible_sets.find(rule.target) == cohort->possible_sets.end()) {
				continue;
			}

			if (cohort->readings.size() == 1) {
				if (type == K_SELECT) {
					continue;
				}
				else if ((type == K_REMOVE || type == K_IFF) && (!unsafe || (rule.flags & RF_SAFE)) && !(rule.flags & RF_UNSAFE)) {
					continue;
				}
			}
			if (type == K_DELIMIT && c == current.cohorts.size()-1) {
				continue;
			}
			if (rule.wordform && rule.wordform != cohort->wordform) {
				rule.num_fail++;
				continue;
			}

			if (rule.flags & RF_ENCL_INNER) {
				if (!par_left_pos) {
					continue;
				}
				if (cohort->local_number < par_left_pos || cohort->local_number > par_right_pos) {
					continue;
				}
			}
			else if (rule.flags & RF_ENCL_OUTER) {
				if (par_left_pos && cohort->local_number >= par_left_pos && cohort->local_number <= par_right_pos) {
					continue;
				}
			}

			size_t num_active = 0;
			size_t num_iff = 0;

			if (rule.type == K_IFF) {
				type = K_REMOVE;
			}

			bool did_test = false;
			bool test_good = false;

			foreach (ReadingList, cohort->readings, rter1, rter1_end) {
				Reading *reading = *rter1;
				reading->matched_target = false;
				reading->matched_tests = false;

				if (reading->mapped && (rule.type == K_MAP || rule.type == K_ADD || rule.type == K_REPLACE)) {
					continue;
				}
				if (reading->noprint && !allow_magic_readings) {
					continue;
				}

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
				if (rule.target && doesSetMatchReading(*reading, rule.target, set.is_child_unified|set.is_special)) {
					target = cohort;
					reading->matched_target = true;
					bool good = true;
					if (!did_test) {
						ContextualTest *test = rule.test_head;
						while (test) {
							if (rule.flags & RF_RESETX || !(rule.flags & RF_REMEMBERX)) {
								mark = cohort;
							}
							dep_deep_seen.clear();
							if (!(test->pos & POS_PASS_ORIGIN) && (no_pass_origin || (test->pos & POS_NO_PASS_ORIGIN))) {
								test_good = (runContextualTest(&current, c, test, 0, cohort) != 0);
							}
							else {
								test_good = (runContextualTest(&current, c, test) != 0);
							}
							if (!test_good) {
								good = test_good;
								if (test != rule.test_head && !(rule.flags & (RF_REMEMBERX|RF_KEEPORDER))) {
									test->detach();
									if (rule.test_head) {
										rule.test_head->prev = test;
										test->next = rule.test_head;
									}
									rule.test_head = test;
								}
								break;
							}
							test = test->next;
						}
					}
					else if (did_test) {
						good = test_good;
					}
					if (good) {
						if (rule.type == K_IFF) {
							type = K_SELECT;
						}
						reading->matched_tests = true;
						num_active++;
						rule.num_match++;
					}
					num_iff++;
				}
				else {
					rule.num_fail++;
				}
			}

			if (num_active == 0 && (num_iff == 0 || rule.type != K_IFF)) {
				continue;
			}
			if (num_active == cohort->readings.size()) {
				if (rule.type == K_SELECT) {
					continue;
				}
				else if (rule.type == K_REMOVE && (!unsafe || (rule.flags & RF_SAFE)) && !(rule.flags & RF_UNSAFE)) {
					continue;
				}
			}

			uint32_t did_append = 0;
			ReadingList removed;
			ReadingList selected;

			// ToDo: Test APPEND followed by MAP
			foreach (ReadingList, cohort->readings, rter2, rter2_end) {
				Reading &reading = **rter2;
				bool good = reading.matched_tests;

				if (rule.type == K_IFF && type == K_REMOVE && reading.matched_target) {
					rule.num_match++;
					good = true;
				}

				if (type == K_REMOVE) {
					if (good) {
						removed.push_back(&reading);
						reading.deleted = true;
						reading.hit_by.push_back(rule.line);
						section_did_good = true;
					}
				}
				else if (type == K_SELECT) {
					if (good) {
						selected.push_back(&reading);
						reading.hit_by.push_back(rule.line);
					}
					else {
						removed.push_back(&reading);
						reading.deleted = true;
						reading.hit_by.push_back(rule.line);
					}
					if (good) {
						section_did_good = true;
					}
				}
				else if (good) {
					section_did_good = false;
					if (type == K_REMVARIABLE) {
						u_fprintf(ux_stderr, "Info: RemVariable fired for %u.\n", rule.varname);
						variables.erase(rule.varname);
					}
					else if (type == K_SETVARIABLE) {
						u_fprintf(ux_stderr, "Info: SetVariable fired for %u.\n", rule.varname);
						variables[rule.varname] = 1;
					}
					else if (type == K_DELIMIT) {
						SingleWindow *nwin = current.parent->allocPushSingleWindow();

						current.parent->cohort_counter++;
						Cohort *cCohort = new Cohort(nwin);
						cCohort->global_number = 0;
						cCohort->wordform = begintag;

						Reading *cReading = new Reading(cCohort);
						cReading->baseform = begintag;
						cReading->wordform = begintag;
						if (grammar->sets_any && !grammar->sets_any->empty()) {
							cReading->parent->possible_sets.insert(grammar->sets_any->begin(), grammar->sets_any->end());
						}
						addTagToReading(*cReading, begintag);

						cCohort->appendReading(cReading);

						nwin->appendCohort(cCohort);

						size_t nc = c+1;
						for ( ; nc < current.cohorts.size() ; nc++) {
							current.cohorts.at(nc)->parent = nwin;
							nwin->appendCohort(current.cohorts.at(nc));
						}
						c = current.cohorts.size()-c;
						for (nc = 0 ; nc < c-1 ; nc++) {
							current.cohorts.pop_back();
						}

						cohort = current.cohorts.back();
						foreach (ReadingList, cohort->readings, rter3, rter3_end) {
							Reading *reading = *rter3;
							addTagToReading(*reading, endtag);
						}
						delimited = true;
						rebuildCohortLinks();
						break;
					}
					else if (rule.type == K_ADD || rule.type == K_MAP) {
						reading.hit_by.push_back(rule.line);
						reading.noprint = false;
						TagList mappings;
						const_foreach (TagList, rule.maplist, tter, tter_end) {
							if ((*tter)->type & T_MAPPING || (*tter)->tag[0] == grammar->mapping_prefix) {
								mappings.push_back(*tter);
							}
							else {
								addTagToReading(reading, (*tter)->hash);
							}
							updateValidRules(rules, intersects, (*tter)->hash, reading);
						}
						if (!mappings.empty()) {
							splitMappings(mappings, *cohort, reading, rule.type == K_MAP);
						}
						if (rule.type == K_MAP) {
							reading.mapped = true;
						}
					}
					else if (rule.type == K_REPLACE) {
						reading.hit_by.push_back(rule.line);
						reading.noprint = false;
						reading.tags_list.clear();
						reading.tags_list.push_back(reading.wordform);
						reading.tags_list.push_back(reading.baseform);
						TagList mappings;
						const_foreach (TagList, rule.maplist, tter, tter_end) {
							if ((*tter)->type & T_MAPPING || (*tter)->tag[0] == grammar->mapping_prefix) {
								mappings.push_back(*tter);
							}
							else {
								reading.tags_list.push_back((*tter)->hash);
							}
							updateValidRules(rules, intersects, (*tter)->hash, reading);
						}
						reflowReading(reading);
						if (!mappings.empty()) {
							splitMappings(mappings, *cohort, reading, true);
						}
					}
					else if (rule.type == K_SUBSTITUTE) {
						// ToDo: Check whether this substitution will do nothing at all to the end result
						uint32_t tloc = 0;
						size_t tagb = reading.tags_list.size();
						const_foreach (uint32List, rule.sublist, tter, tter_end) {
							if (!tloc) {
								foreach (uint32List, reading.tags_list, tfind, tfind_end) {
									if (*tfind == *tter) {
										tloc = *(--tfind);
										break;
									}
								}
							}
							reading.tags_list.remove(*tter);
							reading.tags.erase(*tter);
							if (reading.baseform == *tter) {
								reading.baseform = 0;
							}
						}
						if (tagb != reading.tags_list.size()) {
							reading.hit_by.push_back(rule.line);
							reading.noprint = false;
							foreach (uint32List, reading.tags_list, tfind, tfind_end) {
								if (*tfind == tloc) {
									tfind++;
									break;
								}
							}
							TagList mappings;
							const_foreach (TagList, rule.maplist, tter, tter_end) {
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
									reading.tags_list.insert(tfind, (*tter)->hash);
								}
								updateValidRules(rules, intersects, (*tter)->hash, reading);
							}
							reflowReading(reading);
							if (!mappings.empty()) {
								splitMappings(mappings, *cohort, reading, true);
							}
						}
					}
					else if (rule.type == K_APPEND && rule.line != did_append) {
						Reading *cReading = cohort->allocateAppendReading();
						numReadings++;
						cReading->hit_by.push_back(rule.line);
						cReading->noprint = false;
						addTagToReading(*cReading, cohort->wordform);
						TagList mappings;
						const_foreach (TagList, rule.maplist, tter, tter_end) {
							if ((*tter)->type & T_MAPPING || (*tter)->tag[0] == grammar->mapping_prefix) {
								mappings.push_back(*tter);
							}
							else {
								addTagToReading(*cReading, (*tter)->hash);
							}
							updateValidRules(rules, intersects, (*tter)->hash, reading);
						}
						if (!mappings.empty()) {
							splitMappings(mappings, *cohort, *cReading, true);
						}
						did_append = rule.line;
					}
					else if (type == K_SETPARENT || type == K_SETCHILD) {
						int32_t orgoffset = rule.dep_target->offset;
						uint32Set seen_targets;

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
								ContextualTest *test = rule.dep_test_head;
								while (test) {
									mark = attach;
									dep_deep_seen.clear();
									test_good = (runContextualTest(attach->parent, attach->local_number, test) != 0);
									if (!test_good) {
										good = test_good;
										break;
									}
									test = test->next;
								}
								if (good) {
									if (type == K_SETPARENT) {
										attached = attachParentChild(*attach, *cohort, (rule.flags & RF_ALLOWLOOP) != 0, (rule.flags & RF_ALLOWCROSS) != 0);
									}
									else {
										attached = attachParentChild(*cohort, *attach, (rule.flags & RF_ALLOWLOOP) != 0, (rule.flags & RF_ALLOWCROSS) != 0);
									}
									if (attached) {
										reading.hit_by.push_back(rule.line);
										reading.noprint = false;
										has_dep = true;
									}
								}
								if (rule.flags & RF_NEAREST) {
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
							ContextualTest *test = rule.dep_test_head;
							while (test) {
								mark = attach;
								dep_deep_seen.clear();
								test_good = (runContextualTest(attach->parent, attach->local_number, test) != 0);
								if (!test_good) {
									good = test_good;
									break;
								}
								test = test->next;
							}
							uint32_t a = cohort->local_number;
							uint32_t b = attach->local_number;
							uint32_t max = current.cohorts.size()-1;
							if (type == K_MOVE_BEFORE && b == 0) {
								good = false;
							}
							else if (type == K_MOVE_BEFORE) {
								b--;
							}
							if (good && a != b) {
								if (type == K_SWITCH && a != 0 && b != 0) {
									current.cohorts[a] = attach;
									current.cohorts[b] = cohort;
								}
								else {
									if (a != max) {
										for (uint32_t i = a ; i < max ; i++) {
											current.cohorts[i] = current.cohorts[i+1];
										}
									}
									current.cohorts[max] = 0;
									if (b != max) {
										for (uint32_t i = max ; i > b ; i--) {
											current.cohorts[i] = current.cohorts[i-1];
										}
									}
									current.cohorts[b] = 0;
									current.cohorts[b] = cohort;
								}
								for (uint32_t i = 0 ; i <= max ; i++) {
									current.cohorts[i]->local_number = i;
								}
								a=a;
							}
						}
						rebuildCohortLinks();
						break;
					}
					else if (type == K_ADDRELATION || type == K_SETRELATION || type == K_REMRELATION) {
						Cohort *attach = 0;
						dep_deep_seen.clear();
						attach_to = 0;
						if (runContextualTest(&current, c, rule.dep_target, &attach) && attach) {
							if (attach_to) {
								attach = attach_to;
							}
							bool good = true;
							ContextualTest *test = rule.dep_test_head;
							while (test) {
								mark = attach;
								dep_deep_seen.clear();
								test_good = (runContextualTest(attach->parent, attach->local_number, test) != 0);
								if (!test_good) {
									good = test_good;
									break;
								}
								test = test->next;
							}
							if (good) {
								reading.hit_by.push_back(rule.line);
								reading.noprint = false;
								if (type == K_ADDRELATION) {
									attach->is_related = true;
									cohort->is_related = true;
									cohort->addRelation(rule.maplist.front()->hash, attach->global_number);
								}
								else if (type == K_SETRELATION) {
									attach->is_related = true;
									cohort->is_related = true;
									cohort->setRelation(rule.maplist.front()->hash, attach->global_number);
								}
								else {
									cohort->remRelation(rule.maplist.front()->hash, attach->global_number);
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
							ContextualTest *test = rule.dep_test_head;
							while (test) {
								mark = attach;
								dep_deep_seen.clear();
								test_good = (runContextualTest(attach->parent, attach->local_number, test) != 0);
								if (!test_good) {
									good = test_good;
									break;
								}
								test = test->next;
							}
							if (good) {
								reading.hit_by.push_back(rule.line);
								reading.noprint = false;
								if (type == K_ADDRELATIONS) {
									attach->is_related = true;
									cohort->is_related = true;
									cohort->addRelation(rule.maplist.front()->hash, attach->global_number);
									attach->addRelation(rule.sublist.front(), cohort->global_number);
								}
								else if (type == K_SETRELATIONS) {
									attach->is_related = true;
									cohort->is_related = true;
									cohort->setRelation(rule.maplist.front()->hash, attach->global_number);
									attach->setRelation(rule.sublist.front(), cohort->global_number);
								}
								else {
									cohort->remRelation(rule.maplist.front()->hash, attach->global_number);
									attach->remRelation(rule.sublist.front(), cohort->global_number);
								}
							}
						}
					}
				}
			}

			// ToDo: SETRELATION and others may block for reruns...
			if (single_run) {
				section_did_good = false;
			}

			if (!removed.empty()) {
				if (rule.flags & RF_DELAYED) {
					cohort->delayed.insert(cohort->delayed.end(), removed.begin(), removed.end());
				}
				else {
					cohort->deleted.insert(cohort->deleted.end(), removed.begin(), removed.end());
				}
				while (!removed.empty()) {
					cohort->readings.remove(removed.back());
					removed.pop_back();
				}
				cohort->num_is_current = false;
				section_did_good = true;
			}
			if (!selected.empty()) {
				cohort->readings = selected;
				cohort->num_is_current = false;
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

	if (section_did_good) {
		retval |= RV_SOMETHING;
	}
	if (delimited) {
		retval |= RV_DELIMITED;
	}
	return retval;
}

int GrammarApplicator::runGrammarOnSingleWindow(SingleWindow &current) {
	if (!grammar->before_sections.empty() && !no_before_sections) {
		uint32_t rv = runRulesOnWindow(current, *(runsections[-1]));
		if (rv & RV_DELIMITED) {
			return rv;
		}
	}

	if (!grammar->rules.empty() && !no_sections) {
		RSType::iterator iter_end = runsections.end();
		if (single_run) {
			--iter_end;
			runRulesOnWindow(current, *(iter_end->second));
		}
		else {
			RSType::iterator iter = runsections.begin();
			for (; iter != iter_end ;) {
				if (iter->first < 0) {
					++iter;
					continue;
				}
				uint32_t rv = 0;
				rv = runRulesOnWindow(current, *(iter->second));
				if (rv & RV_DELIMITED) {
					return rv;
				}
				if (!(rv & RV_SOMETHING)) {
					++iter;
				}
			}
		}
	}

	if (!grammar->after_sections.empty() && !no_after_sections) {
		uint32_t rv = runRulesOnWindow(current, *(runsections[-2]));
		if (rv & RV_DELIMITED) {
			return rv;
		}
	}

	return 0;
}

int GrammarApplicator::runGrammarOnWindow() {
	SingleWindow *current = gWindow->current;
	did_final_enclosure = false;

	if (has_dep) {
		reflowDependencyWindow();
		gWindow->dep_map.clear();
		gWindow->dep_window.clear();
		dep_highest_seen = 0;
	}

	indexSingleWindow(*current);

	has_enclosures = false;
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
						(*eiter)->is_enclosed = true;
					}
					foreach (CohortVector, c->enclosed, eiter2, eiter2_end) {
						encs.push_back(*eiter2);
					}
					c->enclosed = encs;
					has_enclosures = true;
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
	current = gWindow->current;

	++pass;
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

	int rv = runGrammarOnSingleWindow(*current);
	if (rv & RV_DELIMITED) {
		goto label_runGrammarOnWindow_begin;
	}

	if (!grammar->parentheses.empty() && has_enclosures) {
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
					current->cohorts[i+j+1]->is_enclosed = false;
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

	return 0;
}

}

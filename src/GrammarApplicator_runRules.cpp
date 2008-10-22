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

#include "GrammarApplicator.h"

using namespace CG3;
using namespace CG3::Strings;

inline void GrammarApplicator::updateValidRules(uint32Set *rules, uint32Set *intersects, uint32_t hash, Reading *reading) {
	if (grammar->rules_by_tag.find(hash) != grammar->rules_by_tag.end()) {
		SingleWindow *current = reading->parent->parent;
		Cohort *c = reading->parent;
		const_foreach(uint32HashSet, (*(grammar->rules_by_tag.find(hash)->second)), rsit, rsit_end) {
			const Rule *r = grammar->rule_by_line.find(*rsit)->second;
			if (r->wordform && r->wordform != c->wordform) {
				continue;
			}
			if (current->rule_to_cohorts.find(*rsit) == current->rule_to_cohorts.end()) {
				current->rule_to_cohorts[r->line] = new uint32Set;
			}
			uint32Set *s = current->rule_to_cohorts[*rsit];
			s->insert(c->global_number);
			current->valid_rules.insert(r->line);
		}

		//valid_rules->insert(grammar->rules_by_tag.find(hash)->second->begin(), grammar->rules_by_tag.find(hash)->second->end());
		uint32Set tmp;
		std::set_intersection(rules->begin(), rules->end(),
			current->valid_rules.begin(), current->valid_rules.end(),
			std::inserter(tmp, tmp.begin()));
		intersects->insert(tmp.begin(), tmp.end());
	}
}

uint32_t GrammarApplicator::runRulesOnWindow(SingleWindow *current, uint32Set *rules) {

	Recycler *r = Recycler::instance();
	uint32_t retval = RV_NOTHING;
	bool section_did_good = false;
	bool delimited = false;

	uint32Set intersects;
	std::set_intersection(rules->begin(), rules->end(),
		current->valid_rules.begin(), current->valid_rules.end(),
		std::inserter(intersects, intersects.begin()));

	foreach (uint32Set, intersects, iter_rules, iter_rules_end) {
		uint32_t j = (*iter_rules);
		const Rule *rule = grammar->rule_by_line.find(j)->second;

		clock_t tstamp = 0;
		KEYWORDS type = rule->type;

		if (!apply_mappings && (rule->type == K_MAP || rule->type == K_ADD || rule->type == K_REPLACE)) {
			continue;
		}
		if (!apply_corrections && (rule->type == K_SUBSTITUTE || rule->type == K_APPEND)) {
			continue;
		}
		if (statistics) {
			tstamp = clock();
		}

		const Set *set = grammar->sets_by_contents.find(rule->target)->second;

		// ToDo: Update list of in/valid rules upon MAP, ADD, REPLACE, APPEND, SUBSTITUTE; add tags + always add tag_any

		//for (size_t c=1 ; c < current->cohorts.size() ; c++) {
		foreach(uint32Set, (*(current->rule_to_cohorts.find(rule->line)->second)), rocit, rocit_end) {
			if (current->parent->cohort_map.find(*rocit) == current->parent->cohort_map.end()) {
				continue;
			}
			Cohort *cohort = current->parent->cohort_map.find(*rocit)->second;
			uint32_t c = cohort->local_number;
			if (cohort->is_enclosed || cohort->parent != current) {
				continue;
			}
			if (cohort->readings.empty()) {
				continue;
			}
			if (cohort->possible_sets.find(rule->target) == cohort->possible_sets.end()) {
				continue;
			}

			if ((type == K_SELECT || type == K_REMOVE || type == K_IFF) && (cohort->is_disamb || cohort->readings.size() == 1)) {
				continue;
			}
			if (type == K_DELIMIT && c == current->cohorts.size()-1) {
				continue;
			}
			if (rule->wordform && rule->wordform != cohort->wordform) {
				rule->num_fail++;
				continue;
			}

			size_t num_active = 0;
			size_t num_iff = 0;
			bool all_active = false;

			if (rule->type == K_IFF) {
				type = K_REMOVE;
			}

			bool good_mapping = false;
			bool did_test = false;
			bool test_good = false;

			foreach (std::list<Reading*>, cohort->readings, rter1, rter1_end) {
				Reading *reading = *rter1;
				reading->matched_target = false;
				reading->matched_tests = false;

				if (reading->mapped && (rule->type == K_MAP || rule->type == K_ADD || rule->type == K_REPLACE)) {
					continue;
				}
				if (reading->noprint && !allow_magic_readings) {
					continue;
				}

				// ToDo: Enable/Disable unif_mode per top-level set
				unif_mode = false;
				unif_last_wordform = 0;
				unif_last_baseform = 0;
				unif_last_textual = 0;
				if (!unif_tags.empty()) {
					unif_tags.clear();
				}

				if (rule->target && doesSetMatchReading(reading, rule->target, set->is_child_unified|set->is_special)) {
					reading->matched_target = true;
					bool good = true;
					if (!rule->tests.empty() && !did_test) {
						foreach (std::list<ContextualTest*>, rule->tests, iter, iter_end) {
							ContextualTest *test = *iter;
							if (!(test->pos & POS_PASS_ORIGIN) && (no_pass_origin || (test->pos & POS_NO_PASS_ORIGIN))) {
								test_good = (runContextualTest(current, c, test, 0, cohort) != 0);
							}
							else {
								test_good = (runContextualTest(current, c, test) != 0);
							}
							if (!test_good) {
								good = test_good;
								if (!statistics) {
									break;
								}
							}
						}
					}
					else if (did_test) {
						good = test_good;
					}
					if (good) {
						if (rule->type == K_IFF) {
							type = K_SELECT;
						}
						reading->matched_tests = true;
						num_active++;
						rule->num_match++;
					}
					num_iff++;
				}
				else {
					rule->num_fail++;
				}
			}

			if (num_active == 0 && (num_iff == 0 || rule->type != K_IFF)) {
				continue;
			}
			if (num_active == cohort->readings.size()) {
				all_active = true;
			}
			if (all_active && (rule->type == K_SELECT || rule->type == K_REMOVE)) {
				continue;
			}

			uint32_t did_append = 0;
			std::list<Reading*> removed;
			std::list<Reading*> selected;

			// ToDo: Test APPEND followed by MAP
			foreach (std::list<Reading*>, cohort->readings, rter2, rter2_end) {
				Reading *reading = *rter2;
				bool good = reading->matched_tests;

				if (rule->type == K_IFF && type == K_REMOVE && reading->matched_target) {
					rule->num_match++;
					good = true;
				}

				if (type == K_REMOVE) {
					if (good) {
						removed.push_back(reading);
						reading->deleted = true;
						reading->hit_by.push_back(rule->line);
						section_did_good = true;
					}
				}
				else if (type == K_SELECT) {
					if (good) {
						selected.push_back(reading);
						reading->hit_by.push_back(rule->line);
					}
					else {
						removed.push_back(reading);
						reading->deleted = true;
						reading->hit_by.push_back(rule->line);
					}
					if (good) {
						section_did_good = true;
					}
				}
				else if (good) {
					if (type == K_REMVARIABLE) {
						u_fprintf(ux_stderr, "Info: RemVariable fired for %u.\n", rule->varname);
						variables.erase(rule->varname);
					}
					else if (type == K_SETVARIABLE) {
						u_fprintf(ux_stderr, "Info: SetVariable fired for %u.\n", rule->varname);
						variables[rule->varname] = 1;
					}
					else if (type == K_DELIMIT) {
						SingleWindow *nwin = new SingleWindow(current->parent);
						current->parent->pushSingleWindow(nwin);

						current->parent->cohort_counter++;
						Cohort *cCohort = r->new_Cohort(nwin);
						cCohort->global_number = 0;
						cCohort->wordform = begintag;

						Reading *cReading = r->new_Reading(cCohort);
						cReading->baseform = begintag;
						cReading->wordform = begintag;
						if (grammar->sets_by_tag.find(grammar->tag_any) != grammar->sets_by_tag.end()) {
							cReading->parent->possible_sets.insert(grammar->sets_by_tag.find(grammar->tag_any)->second->begin(), grammar->sets_by_tag.find(grammar->tag_any)->second->end());
							cReading->possible_sets.insert(grammar->sets_by_tag.find(grammar->tag_any)->second->begin(), grammar->sets_by_tag.find(grammar->tag_any)->second->end());
						}
						addTagToReading(cReading, begintag);

						cCohort->appendReading(cReading);

						nwin->appendCohort(cCohort);

						size_t nc = c+1;
						for ( ; nc < current->cohorts.size() ; nc++) {
							current->cohorts.at(nc)->parent = nwin;
							nwin->appendCohort(current->cohorts.at(nc));
						}
						c = current->cohorts.size()-c;
						for (nc = 0 ; nc < c-1 ; nc++) {
							current->cohorts.pop_back();
						}

						cohort = current->cohorts.back();
						foreach (std::list<Reading*>, cohort->readings, rter3, rter3_end) {
							Reading *reading = *rter3;
							addTagToReading(reading, endtag);
						}
						delimited = true;
						break;
					}
					else if (rule->type == K_ADD || rule->type == K_MAP) {
						reading->hit_by.push_back(rule->line);
						reading->noprint = false;
						TagList mappings;
						const_foreach (TagList, rule->maplist, tter, tter_end) {
							if ((*tter)->type & T_MAPPING || (*tter)->tag[0] == grammar->mapping_prefix) {
								mappings.push_back(*tter);
							}
							else {
								addTagToReading(reading, (*tter)->hash);
							}
							updateValidRules(rules, &intersects, (*tter)->hash, reading);
						}
						if (!mappings.empty()) {
							splitMappings(mappings, cohort, reading, rule->type == K_MAP);
						}
						if (rule->type == K_MAP) {
							reading->mapped = true;
						}
					}
					else if (rule->type == K_REPLACE) {
						reading->hit_by.push_back(rule->line);
						reading->noprint = false;
						reading->tags_list.clear();
						reading->tags_list.push_back(reading->wordform);
						reading->tags_list.push_back(reading->baseform);
						TagList mappings;
						const_foreach (TagList, rule->maplist, tter, tter_end) {
							if ((*tter)->type & T_MAPPING || (*tter)->tag[0] == grammar->mapping_prefix) {
								mappings.push_back(*tter);
							}
							else {
								reading->tags_list.push_back((*tter)->hash);
							}
							updateValidRules(rules, &intersects, (*tter)->hash, reading);
						}
						reflowReading(reading);
						if (!mappings.empty()) {
							splitMappings(mappings, cohort, reading, true);
						}
					}
					else if (rule->type == K_SUBSTITUTE) {
						uint32_t tloc = 0;
						size_t tagb = reading->tags_list.size();
						const_foreach (uint32List, rule->sublist, tter, tter_end) {
							if (!tloc) {
								foreach (uint32List, reading->tags_list, tfind, tfind_end) {
									if (*tfind == *tter) {
										tloc = *(--tfind);
										break;
									}
								}
							}
							reading->tags_list.remove(*tter);
							if (reading->baseform == *tter) {
								reading->baseform = 0;
							}
						}
						if (tagb != reading->tags_list.size()) {
							reading->hit_by.push_back(rule->line);
							reading->noprint = false;
							foreach (uint32List, reading->tags_list, tfind, tfind_end) {
								if (*tfind == tloc) {
									tfind++;
									break;
								}
							}
							TagList mappings;
							const_foreach (TagList, rule->maplist, tter, tter_end) {
								if ((*tter)->hash == grammar->tag_any) {
									break;
								}
								if ((*tter)->type & T_MAPPING || (*tter)->tag[0] == grammar->mapping_prefix) {
									mappings.push_back(*tter);
								}
								else {
									reading->tags_list.insert(tfind, (*tter)->hash);
								}
								updateValidRules(rules, &intersects, (*tter)->hash, reading);
							}
							reflowReading(reading);
							if (!mappings.empty()) {
								splitMappings(mappings, cohort, reading, true);
							}
						}
					}
					else if (rule->type == K_APPEND && rule->line != did_append) {
						reading = cohort->allocateAppendReading();
						numReadings++;
						reading->hit_by.push_back(rule->line);
						reading->noprint = false;
						addTagToReading(reading, cohort->wordform);
						TagList mappings;
						const_foreach (TagList, rule->maplist, tter, tter_end) {
							if ((*tter)->type & T_MAPPING || (*tter)->tag[0] == grammar->mapping_prefix) {
								mappings.push_back(*tter);
							}
							else {
								addTagToReading(reading, (*tter)->hash);
							}
							updateValidRules(rules, &intersects, (*tter)->hash, reading);
						}
						if (!mappings.empty()) {
							splitMappings(mappings, cohort, reading, true);
						}
						did_append = rule->line;
					}
					else if (type == K_SETPARENT || type == K_SETCHILD) {
						Cohort *attach = runContextualTest(current, c, rule->dep_target);
						if (attach) {
							bool good = true;
							if (!rule->dep_tests.empty()) {
								foreach (std::list<ContextualTest*>, rule->dep_tests, iter, iter_end) {
									ContextualTest *test = *iter;
									test_good = (runContextualTest(attach->parent, attach->local_number, test) != 0);
									if (!test_good) {
										good = test_good;
										if (!statistics) {
											break;
										}
									}
								}
							}
							if (good) {
								reading->hit_by.push_back(rule->line);
								has_dep = true;
								if (type == K_SETPARENT) {
									attachParentChild(attach, cohort);
								}
								else {
									attachParentChild(cohort, attach);
								}
							}
						}
						break;
					}
					else if (type == K_SETRELATION || type == K_REMRELATION) {
						Cohort *attach = runContextualTest(current, c, rule->dep_target);
						if (attach) {
							bool good = true;
							if (!rule->dep_tests.empty()) {
								foreach (std::list<ContextualTest*>, rule->dep_tests, iter, iter_end) {
									ContextualTest *test = *iter;
									test_good = (runContextualTest(attach->parent, attach->local_number, test) != 0);
									if (!test_good) {
										good = test_good;
										if (!statistics) {
											break;
										}
									}
								}
							}
							if (good) {
								reading->hit_by.push_back(rule->line);
								if (type == K_SETRELATION) {
									attach->is_related = true;
									cohort->is_related = true;
									cohort->relations.insert( std::pair<uint32_t,uint32_t>(attach->global_number, (rule->maplist.front())->hash) );
								}
								else {
									std::multimap<uint32_t,uint32_t>::iterator miter = cohort->relations.find(attach->global_number);
									while (miter != cohort->relations.end()
										&& miter->first == attach->global_number
										&& miter->second == (rule->maplist.front())->hash) {
											cohort->relations.erase(miter);
											miter = cohort->relations.find(attach->global_number);
									}
								}
							}
						}
						break;
					}
					else if (type == K_SETRELATIONS || type == K_REMRELATIONS) {
						Cohort *attach = runContextualTest(current, c, rule->dep_target);
						if (attach) {
							bool good = true;
							if (!rule->dep_tests.empty()) {
								foreach (std::list<ContextualTest*>, rule->dep_tests, iter, iter_end) {
									ContextualTest *test = *iter;
									test_good = (runContextualTest(attach->parent, attach->local_number, test) != 0);
									if (!test_good) {
										good = test_good;
										if (!statistics) {
											break;
										}
									}
								}
							}
							if (good) {
								reading->hit_by.push_back(rule->line);
								if (type == K_SETRELATIONS) {
									attach->is_related = true;
									cohort->is_related = true;
									cohort->relations.insert( std::pair<uint32_t,uint32_t>(attach->global_number, (rule->maplist.front())->hash) );
									attach->relations.insert( std::pair<uint32_t,uint32_t>(cohort->global_number, rule->sublist.front()) );
								}
								else {
									std::multimap<uint32_t,uint32_t>::iterator miter = cohort->relations.find(attach->global_number);
									while (miter != cohort->relations.end()
										&& miter->first == attach->global_number
										&& miter->second == (rule->maplist.front())->hash) {
											cohort->relations.erase(miter);
											miter = cohort->relations.find(attach->global_number);
									}
									
									miter = attach->relations.find(cohort->global_number);
									while (miter != attach->relations.end()
										&& miter->first == cohort->global_number
										&& miter->second == rule->sublist.front()) {
											attach->relations.erase(miter);
											miter = attach->relations.find(cohort->global_number);
									}
								}
							}
						}
					}
				}
			}

			if (!good_mapping && removed.empty()) {
				section_did_good = false;
			}
			if (single_run) {
				section_did_good = false;
			}

			if (!removed.empty()) {
				cohort->deleted.insert(cohort->deleted.end(), removed.begin(), removed.end());
				while (!removed.empty()) {
					cohort->readings.remove(removed.back());
					removed.pop_back();
				}
			}
			if (!selected.empty()) {
				cohort->readings.clear();
				cohort->readings.insert(cohort->readings.begin(), selected.begin(), selected.end());
			}

			cohort->is_disamb = false;
			if (cohort->readings.size() == 1) {
				cohort->is_disamb = true;
			}

			if (delimited) {
				break;
			}
		}

		if (statistics) {
			rule->total_time += (clock() - tstamp);
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

int GrammarApplicator::runGrammarOnSingleWindow(SingleWindow *current) {
	if (!grammar->before_sections.empty() && !no_before_sections) {
		uint32_t rv = runRulesOnWindow(current, runsections[-1]);
		if (rv & RV_DELIMITED) {
			return rv;
		}
	}

	if (!grammar->rules.empty() && !no_sections) {
		std::map<int32_t, uint32Set*>::iterator iter_end = runsections.end();
		if (single_run) {
			iter_end--;
			runRulesOnWindow(current, iter_end->second);
		}
		else {
			std::map<int32_t, uint32Set*>::iterator iter = runsections.begin();
			for (; iter != iter_end ;) {
				if (iter->first < 0) {
					iter++;
					continue;
				}
				uint32_t rv = 0;
				rv = runRulesOnWindow(current, iter->second);
				if (rv & RV_DELIMITED) {
					return rv;
				}
				if (!(rv & RV_SOMETHING)) {
					iter++;
				}
			}
		}
	}

	if (!grammar->after_sections.empty() && !no_after_sections) {
		uint32_t rv = runRulesOnWindow(current, runsections[-2]);
		if (rv & RV_DELIMITED) {
			return rv;
		}
	}

	return 0;
}

int GrammarApplicator::runGrammarOnWindow(Window *window) {
	if (has_dep) {
		reflowDependencyWindow();
	}

	SingleWindow *current = window->current;

	current->valid_rules.clear();
	foreach(std::vector<Cohort*>, current->cohorts, iter, iter_end) {
		Cohort *c = *iter;
		foreach(uint32HashSet, c->possible_sets, psit, psit_end) {
			if (grammar->rules_by_set.find(*psit) == grammar->rules_by_set.end()) {
				continue;
			}
			const uint32Set *rules = grammar->rules_by_set.find(*psit)->second;
			const_foreach(uint32Set, (*rules), rsit, rsir_end) {
				const Rule *r = grammar->rule_by_line.find(*rsit)->second;
				if (r->wordform && r->wordform != c->wordform) {
					continue;
				}
				if (current->rule_to_cohorts.find(*rsit) == current->rule_to_cohorts.end()) {
					current->rule_to_cohorts[r->line] = new uint32Set;
				}
				uint32Set *s = current->rule_to_cohorts[*rsit];
				s->insert(c->global_number);
				current->valid_rules.insert(r->line);
			}
		}
	}

	if (!grammar->parentheses.empty()) {
		label_scanParentheses:
		reverse_foreach(std::vector<Cohort*>, current->cohorts, iter, iter_end) {
			Cohort *c = *iter;
			uint32Map::const_iterator p = grammar->parentheses.find(c->wordform);
			if (p != grammar->parentheses.end()) {
				std::vector<Cohort*>::iterator right = iter.base();
				right--;
				right--;
				c = *right;
				right++;
				bool found = false;
				for (; right != current->cohorts.end() ; right++) {
					Cohort *s = *right;
					c->enclosed.push_back(s);
					if (s->wordform == p->second) {
						found = true;
						break;
					}
				}
				if (!found) {
					c->enclosed.clear();
				}
				else {
					std::vector<Cohort*>::iterator left = iter.base();
					left--;
					uint32_t lc = (*left)->local_number;
					right++;
					for (; right != current->cohorts.end() ; right++) {
						*left = *right;
						(*left)->local_number = lc;
						lc++;
						left++;
					}
					current->cohorts.resize(current->cohorts.size() - c->enclosed.size());
					foreach(std::vector<Cohort*>, c->enclosed, eiter, eiter_end) {
						(*eiter)->is_enclosed = true;
					}
					goto label_scanParentheses;
				}
			}
		}
	}

	par_left_tag = 0;
	par_right_tag = 0;
	par_left_pos = 0;
	par_right_pos = 0;

label_runGrammarOnWindow_begin:
	current = window->current;

	int rv = runGrammarOnSingleWindow(current);
	if (rv & RV_DELIMITED) {
		goto label_runGrammarOnWindow_begin;
	}

	if (!grammar->parentheses.empty()) {
		size_t nc = current->cohorts.size();
		for (size_t i=0 ; i<nc ; i++) {
			Cohort *c = current->cohorts[i];
			if (!c->enclosed.empty()) {
				current->cohorts.resize(current->cohorts.size() + c->enclosed.size(), 0);
				size_t ne = c->enclosed.size();
				for (size_t j=nc-1 ; j>i ; j--) {
					current->cohorts[j+ne] = current->cohorts[j];
					current->cohorts[j+ne]->local_number = j+ne;
				}
				for (size_t j=0 ; j<ne ; j++) {
					current->cohorts[i+j+1] = c->enclosed[j];
					current->cohorts[i+j+1]->local_number = i+j+1;
					current->cohorts[i+j+1]->parent = current;
					current->cohorts[i+j+1]->is_enclosed = false;
				}
				par_left_tag = c->enclosed[0]->wordform;
				par_right_tag = c->enclosed[ne-1]->wordform;
				par_left_pos = i+1;
				par_right_pos = i+ne;
				c->enclosed.clear();
				goto label_runGrammarOnWindow_begin;
			}
		}
	}

	return 0;
}

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

#include "GrammarApplicator.h"

using namespace CG3;
using namespace CG3::Strings;

uint32_t GrammarApplicator::runRulesOnWindow(SingleWindow *current, const int32_t start, const int32_t end) {

	Recycler *r = Recycler::instance();
	uint32_t retval = RV_NOTHING;
	bool section_did_good = false;
	bool delimited = false;

	uint32Set::iterator iter_rules;
	for (iter_rules = current->valid_rules.begin() ; iter_rules != current->valid_rules.end() ; iter_rules++) {
		uint32_t j = (*iter_rules);
		const Rule *rule = grammar->rule_by_line.find(j)->second;
		if (start == 0 && rule->section < 0) {
			continue;
		}
		if (start == 0 && rule->section > end) {
			break;
		}
		if (rule->section != start && (start == -1 || start == -2)) {
			break;
		}
		PACC_TimeStamp tstamp = 0;
		KEYWORDS type = rule->type;

		if (!apply_mappings && (rule->type == K_MAP || rule->type == K_ADD || rule->type == K_REPLACE)) {
			continue;
		}
		if (!apply_corrections && (rule->type == K_SUBSTITUTE || rule->type == K_APPEND)) {
			continue;
		}
		if (statistics) {
			tstamp = timer->getCount();
		}

		bool rule_is_valid = false;

		for (uint32_t c=1 ; c < current->cohorts.size() ; c++) {
			Cohort *cohort = current->cohorts[c];
			if (cohort->readings.empty()) {
				continue;
			}
			if (cohort->invalid_rules.find(rule->line) != cohort->invalid_rules.end()) {
				skipped_rules++;
				continue;
			}

			const Set *set = grammar->sets_by_contents.find(rule->target)->second;
			if ((type == K_SELECT || type == K_REMOVE || type == K_IFF) && (cohort->is_disamb || cohort->readings.size() == 1)) {
				if (cohort->is_disamb || !set->has_mappings || cohort->readings.front()->tags_mapped->size() <= 1) {
					continue;
				}
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
			std::list<Reading*>::iterator rter;

			if (rule->type == K_IFF) {
				type = K_REMOVE;
			}

			bool rule_is_valid_cohort = false;
			bool good_mapping = false;
			bool did_test = false;
			bool did_append = false;
			bool test_good = false;
			for (rter = cohort->readings.begin() ; rter != cohort->readings.end() ; rter++) {
				Reading *reading = *rter;
				reading->matched_target = false;
				reading->matched_tests = false;
				reading->current_mapping_tag = 0;

				if (reading->mapped && (rule->type == K_MAP || rule->type == K_ADD || rule->type == K_REPLACE)) {
					continue;
				}
				if (reading->noprint && !allow_magic_readings) {
					continue;
				}
				last_mapping_tag = 0;
				if (rule->target && doesSetMatchReading(reading, rule->target, set->has_mappings)) {
					rule_is_valid_cohort = true;
					rule_is_valid = true;
					reading->current_mapping_tag = last_mapping_tag;
					reading->matched_target = true;
					bool good = true;
					if (!rule->tests.empty() && !did_test) {
						std::list<ContextualTest*>::iterator iter;
						for (iter = rule->tests.begin() ; iter != rule->tests.end() ; iter++) {
							ContextualTest *test = *iter;
							test_good = (runContextualTest(current, c, test) != 0);
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
			if (!rule_is_valid_cohort) {
				cohort->invalid_rules.insert(rule->line);
			}

			if (num_active == 0 && (num_iff == 0 || rule->type != K_IFF)) {
				continue;
			}
			if (num_active == cohort->readings.size()) {
				all_active = true;
			}
			if (all_active && rule->type == K_SELECT && !set->has_mappings && !last_mapping_tag) {
				continue;
			}
			if (all_active && rule->type == K_REMOVE && !set->has_mappings && !last_mapping_tag) {
				continue;
			}

			std::list<Reading*> removed;
			std::list<Reading*> selected;

			for (rter = cohort->readings.begin() ; rter != cohort->readings.end() ; rter++) {
				Reading *reading = *rter;
				bool good = reading->matched_tests;

				if (rule->type == K_IFF && type == K_REMOVE && reading->matched_target) {
					rule->num_match++;
					good = true;
				}

				if (type == K_REMOVE) {
					if (good && reading->current_mapping_tag && reading->tags_mapped->size() > 1) {
						delTagFromReading(reading, reading->current_mapping_tag);
						good_mapping = true;
					}
					else {
						if (good) {
							removed.push_back(reading);
							reading->deleted = true;
							reading->hit_by.push_back(rule->line);
						}
					}
					if (good) {
						section_did_good = true;
					}
				}
				else if (type == K_SELECT) {
					if (good && reading->current_mapping_tag && reading->tags_mapped->size() > 1) {
						uint32HashSet::iterator iter_maps;
						while (!reading->tags_mapped->empty()) {
							iter_maps = reading->tags_mapped->begin();
							delTagFromReading(reading, *iter_maps);
						}
						addTagToReading(reading, reading->current_mapping_tag);
						good_mapping = true;
					}
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
							cReading->possible_sets.insert(grammar->sets_by_tag.find(grammar->tag_any)->second->begin(), grammar->sets_by_tag.find(grammar->tag_any)->second->end());
						}
						addTagToReading(cReading, begintag);

						cCohort->appendReading(cReading);

						nwin->appendCohort(cCohort);

						uint32_t nc = c+1;
						for ( ; nc < current->cohorts.size() ; nc++) {
							current->cohorts.at(nc)->parent = nwin;
							nwin->appendCohort(current->cohorts.at(nc));
						}
						c = (uint32_t)current->cohorts.size()-c;
						for (nc = 0 ; nc < c-1 ; nc++) {
							current->cohorts.pop_back();
						}

						cohort = current->cohorts.back();
						for (rter = cohort->readings.begin() ; rter != cohort->readings.end() ; rter++) {
							Reading *reading = *rter;
							addTagToReading(reading, endtag);
						}
						delimited = true;
						break;
					}
					else if (rule->type == K_ADD || rule->type == K_MAP) {
						reading->hit_by.push_back(rule->line);
						reading->noprint = false;
						uint32List::const_iterator tter;
						for (tter = rule->maplist.begin() ; tter != rule->maplist.end() ; tter++) {
							addTagToReading(reading, *tter);
							if (grammar->rules_by_tag.find(*tter) != grammar->rules_by_tag.end()) {
								current->valid_rules.insert(grammar->rules_by_tag.find(*tter)->second->begin(), grammar->rules_by_tag.find(*tter)->second->end());
							}
						}
						if (rule->type == K_MAP) {
							reading->mapped = true;
						}
					}
					else if (rule->type == K_REPLACE) {
						reading->hit_by.push_back(rule->line);
						reading->noprint = false;
						uint32List::const_iterator tter;
						reading->tags_list.clear();
						reading->tags_list.push_back(reading->wordform);
						reading->tags_list.push_back(reading->baseform);
						for (tter = rule->maplist.begin() ; tter != rule->maplist.end() ; tter++) {
							reading->tags_list.push_back(*tter);
							if (grammar->rules_by_tag.find(*tter) != grammar->rules_by_tag.end()) {
								current->valid_rules.insert(grammar->rules_by_tag.find(*tter)->second->begin(), grammar->rules_by_tag.find(*tter)->second->end());
							}
						}
						reflowReading(reading);
						if (!reading->tags_mapped->empty()) {
							reading->mapped = true;
						}
					}
					else if (rule->type == K_SUBSTITUTE) {
						uint32List::const_iterator tter;
						uint32_t tloc = 0;
						size_t tagb = reading->tags_list.size();
						for (tter = rule->sublist.begin() ; tter != rule->sublist.end() ; tter++) {
							if (!tloc) {
								uint32List::iterator tfind;
								for (tfind = reading->tags_list.begin() ; tfind != reading->tags_list.end() ; tfind++) {
									if (*tfind == *tter) {
										tloc = *(--tfind);
										break;
									}
								}
							}
							reading->tags_list.remove(*tter);
						}
						if (tagb != reading->tags_list.size()) {
							reading->hit_by.push_back(rule->line);
							reading->noprint = false;
							uint32List::iterator tfind;
							for (tfind = reading->tags_list.begin() ; tfind != reading->tags_list.end() ; tfind++) {
								if (*tfind == tloc) {
									tfind++;
									break;
								}
							}
							for (tter = rule->maplist.begin() ; tter != rule->maplist.end() ; tter++) {
								reading->tags_list.insert(tfind, *tter);
								if (grammar->rules_by_tag.find(*tter) != grammar->rules_by_tag.end()) {
									current->valid_rules.insert(grammar->rules_by_tag.find(*tter)->second->begin(), grammar->rules_by_tag.find(*tter)->second->end());
								}
							}
							reflowReading(reading);
							if (!reading->tags_mapped->empty()) {
								reading->mapped = true;
							}
						}
					}
					else if (rule->type == K_APPEND && !did_append) {
						Reading *nr = cohort->allocateAppendReading();
						nr->hit_by.push_back(rule->line);
						nr->noprint = false;
						addTagToReading(nr, cohort->wordform);
						uint32List::const_iterator tter;
						for (tter = rule->maplist.begin() ; tter != rule->maplist.end() ; tter++) {
							addTagToReading(nr, *tter);
							if (grammar->rules_by_tag.find(*tter) != grammar->rules_by_tag.end()) {
								current->valid_rules.insert(grammar->rules_by_tag.find(*tter)->second->begin(), grammar->rules_by_tag.find(*tter)->second->end());
							}
						}
						if (!nr->tags_mapped->empty()) {
							nr->mapped = true;
						}
						did_append = true;
					}
					else if (type == K_SETPARENT || type == K_SETCHILD) {
						Cohort *attach = runContextualTest(current, c, rule->dep_target);
						if (attach) {
							bool good = true;
							if (!rule->dep_tests.empty()) {
								std::list<ContextualTest*>::iterator iter;
								for (iter = rule->dep_tests.begin() ; iter != rule->dep_tests.end() ; iter++) {
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
								std::list<ContextualTest*>::iterator iter;
								for (iter = rule->dep_tests.begin() ; iter != rule->dep_tests.end() ; iter++) {
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
									cohort->relations.insert( std::pair<uint32_t,uint32_t>(attach->global_number, rule->maplist.front()) );
								}
								else {
									std::multimap<uint32_t,uint32_t>::iterator miter = cohort->relations.find(attach->global_number);
									while (miter != cohort->relations.end()
										&& miter->first == attach->global_number
										&& miter->second == rule->maplist.front()) {
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
								std::list<ContextualTest*>::iterator iter;
								for (iter = rule->dep_tests.begin() ; iter != rule->dep_tests.end() ; iter++) {
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
									cohort->relations.insert( std::pair<uint32_t,uint32_t>(attach->global_number, rule->maplist.front()) );
									attach->relations.insert( std::pair<uint32_t,uint32_t>(cohort->global_number, rule->sublist.front()) );
								}
								else {
									std::multimap<uint32_t,uint32_t>::iterator miter = cohort->relations.find(attach->global_number);
									while (miter != cohort->relations.end()
										&& miter->first == attach->global_number
										&& miter->second == rule->maplist.front()) {
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
				if (!set->has_mappings || cohort->readings.front()->tags_mapped->size() <= 1) {
					cohort->is_disamb = true;
				}
			}

			if (delimited) {
				break;
			}
		}

		if (!rule_is_valid) {
			if (iter_rules == current->valid_rules.begin()) {
				current->valid_rules.erase(iter_rules);
				iter_rules = current->valid_rules.begin();
			}
			else {
				uint32Set::iterator to_erase = iter_rules;
				uint32_t n = *(--iter_rules);
				current->valid_rules.erase(to_erase);
				iter_rules = current->valid_rules.find(n);
			}
		}

		if (statistics) {
			rule->total_time += (timer->getCount() - tstamp);
		}
		if (delimited) {
			break;
		}
		if (current->valid_rules.empty()) {
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

int GrammarApplicator::runGrammarOnWindow(Window *window) {
label_runGrammarOnWindow_begin:
	if (has_dep) {
		reflowDependencyWindow();
	}
	SingleWindow *current = window->current;

	if (!grammar->before_sections.empty()) {
		uint32_t rv = runRulesOnWindow(current, -1, -1);
		if (rv & RV_DELIMITED) {
			goto label_runGrammarOnWindow_begin;
		}
	}

	if (!grammar->rules.empty()) {
		uint32_t smax = (uint32_t)grammar->sections.size()-1;
		if (sections && sections < smax) {
			smax = sections;
		}
		for (uint32_t i=0;i<smax;) {
			uint32_t rv = runRulesOnWindow(current, 0, i);
			if (rv & RV_DELIMITED) {
				goto label_runGrammarOnWindow_begin;
			}
			if (!(rv & RV_SOMETHING)) {
				i++;
			}
		}
	}

	if (!grammar->after_sections.empty()) {
		uint32_t rv = runRulesOnWindow(current, -2, -2);
		if (rv & RV_DELIMITED) {
			goto label_runGrammarOnWindow_begin;
		}
	}

	return 0;
}

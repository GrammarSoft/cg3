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

uint32_t GrammarApplicator::runRulesOnWindow(SingleWindow *current, const std::vector<Rule*> *rules, const uint32_t start, const uint32_t end) {

	uint32_t retval = RV_NOTHING;
	bool section_did_good = false;
	bool delimited = false;

	if (!rules->empty()) {
		for (uint32_t j=start;j<end;j++) {
			PACC_TimeStamp tstamp = 0;
			const Rule *rule = rules->at(j);
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

			for (uint32_t c=0 ; c < current->cohorts.size() ; c++) {
				if (c == 0) {
					continue;
				}
				Cohort *cohort = current->cohorts[c];
				if (cohort->readings.empty()) {
					continue;
				}

				const Set *set = grammar->sets_by_contents.find(rule->target)->second;
				if ((type == K_SELECT || type == K_REMOVE || type == K_IFF) && cohort->readings.size() == 1) {
					if (!set->has_mappings || cohort->readings.front()->tags_mapped.size() <= 1) {
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

				bool good_mapping = false;
				bool did_test = false;
				bool did_append = false;
				bool test_good = false;
				for (rter = cohort->readings.begin() ; rter != cohort->readings.end() ; rter++) {
					Reading *reading = *rter;
					reading->matched_target = false;
					reading->matched_tests = false;
					reading->current_mapping_tag = 0;
					if (!reading->hash) {
						reading->rehash();
					}
					if (reading->mapped && (rule->type == K_MAP || rule->type == K_ADD || rule->type == K_REPLACE)) {
						continue;
					}
					if (reading->noprint && !allow_magic_readings) {
						continue;
					}
					last_mapping_tag = 0;
					if (rule->target && doesSetMatchReading(reading, rule->target, set->has_mappings)) {
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
				removed.clear();
				selected.clear();

				for (rter = cohort->readings.begin() ; rter != cohort->readings.end() ; rter++) {
					Reading *reading = *rter;
					bool good = reading->matched_tests;

					if (rule->type == K_IFF && type == K_REMOVE && reading->matched_target) {
						rule->num_match++;
						good = true;
					}

					if (type == K_REMOVE) {
						if (good && reading->current_mapping_tag && reading->tags_mapped.size() > 1) {
							reading->tags_list.remove(reading->current_mapping_tag);
							reflowReading(reading);
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
						if (good && reading->current_mapping_tag && reading->tags_mapped.size() > 1) {
							std::map<uint32_t, uint32_t>::iterator iter_maps;
							for (iter_maps = reading->tags_mapped.begin() ; iter_maps != reading->tags_mapped.end() ; iter_maps++) {
								reading->tags_list.remove(iter_maps->second);
							}
							reading->tags_list.push_back(reading->current_mapping_tag);
							reflowReading(reading);
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
							Cohort *cCohort = new Cohort(nwin);
							cCohort->global_number = 0;
							cCohort->wordform = begintag;

							Reading *cReading = new Reading(cCohort);
							cReading->baseform = begintag;
							cReading->wordform = begintag;
							cReading->tags[begintag] = begintag;
							cReading->tags_list.push_back(begintag);
							cReading->rehash();

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
								reading->tags_list.push_back(endtag);
								reading->tags[endtag] = endtag;
								reflowReading(reading);
							}
							delimited = true;
							break;
						}
						else if (rule->type == K_ADD || rule->type == K_MAP) {
							reading->hit_by.push_back(rule->line);
							reading->noprint = false;
							std::list<uint32_t>::const_iterator tter;
							for (tter = rule->maplist.begin() ; tter != rule->maplist.end() ; tter++) {
								reading->tags_list.push_back(*tter);
							}
							reflowReading(reading);
							if (rule->type == K_MAP) {
								reading->mapped = true;
							}
						}
						else if (rule->type == K_REPLACE) {
							reading->hit_by.push_back(rule->line);
							reading->noprint = false;
							std::list<uint32_t>::const_iterator tter;
							reading->tags_list.clear();
							reading->tags_list.push_back(reading->wordform);
							reading->tags_list.push_back(reading->baseform);
							for (tter = rule->maplist.begin() ; tter != rule->maplist.end() ; tter++) {
								reading->tags_list.push_back(*tter);
							}
							reflowReading(reading);
							if (!reading->tags_mapped.empty()) {
								reading->mapped = true;
							}
						}
						else if (rule->type == K_SUBSTITUTE) {
							std::list<uint32_t>::const_iterator tter;
							uint32_t tloc = 0;
							size_t tagb = reading->tags_list.size();
							for (tter = rule->sublist.begin() ; tter != rule->sublist.end() ; tter++) {
								if (!tloc) {
									std::list<uint32_t>::iterator tfind;
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
								std::list<uint32_t>::iterator tfind;
								for (tfind = reading->tags_list.begin() ; tfind != reading->tags_list.end() ; tfind++) {
									if (*tfind == tloc) {
										tfind++;
										break;
									}
								}
								for (tter = rule->maplist.begin() ; tter != rule->maplist.end() ; tter++) {
									reading->tags_list.insert(tfind, *tter);
								}
								reflowReading(reading);
								if (!reading->tags_mapped.empty()) {
									reading->mapped = true;
								}
							}
						}
						else if (rule->type == K_APPEND && !did_append) {
							Reading *nr = cohort->allocateAppendReading();
							nr->hit_by.push_back(rule->line);
							nr->noprint = false;
							nr->tags_list.push_back(cohort->wordform);
							std::list<uint32_t>::const_iterator tter;
							for (tter = rule->maplist.begin() ; tter != rule->maplist.end() ; tter++) {
								nr->tags_list.push_back(*tter);
							}
							reflowReading(nr);
							if (!nr->tags_mapped.empty()) {
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
					removed.clear();
				}
				if (!selected.empty()) {
					cohort->readings.clear();
					cohort->readings.insert(cohort->readings.begin(), selected.begin(), selected.end());
					selected.clear();
				}

				if (delimited) {
					break;
				}
			}

			if (statistics) {
				rule->total_time += (timer->getCount() - tstamp);
			}
			if (delimited) {
				break;
			}
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
		uint32_t rv = runRulesOnWindow(current, &grammar->before_sections, 0, (uint32_t)grammar->before_sections.size());
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
			uint32_t rv = runRulesOnWindow(current, &grammar->rules, 0, grammar->sections[i+1]);
			if (rv & RV_DELIMITED) {
				goto label_runGrammarOnWindow_begin;
			}
			if (!(rv & RV_SOMETHING)) {
				i++;
			}
		}
	}

	if (!grammar->after_sections.empty()) {
		uint32_t rv = runRulesOnWindow(current, &grammar->after_sections, 0, (uint32_t)grammar->after_sections.size());
		if (rv & RV_DELIMITED) {
			goto label_runGrammarOnWindow_begin;
		}
	}

	return 0;
}

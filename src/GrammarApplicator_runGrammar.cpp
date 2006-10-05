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
#include "Window.h"
#include "SingleWindow.h"
#include "uextras.h"

using namespace CG3;
using namespace CG3::Strings;

inline void GrammarApplicator::reflowSingleWindow(SingleWindow *swindow) {
	swindow->tags.clear();
	swindow->tags_mapped.clear();
	swindow->tags_plain.clear();
	swindow->tags_textual.clear();

	for (uint32_t c=0 ; c < swindow->cohorts.size() ; c++) {
		Cohort *cohort = swindow->cohorts[c];

		std::list<Reading*>::iterator rter;
		for (rter = cohort->readings.begin() ; rter != cohort->readings.end() ; rter++) {
			Reading *reading = *rter;

			std::list<uint32_t>::const_iterator tter;
			for (tter = reading->tags_list.begin() ; tter != reading->tags_list.end() ; tter++) {
				swindow->tags[*tter] = *tter;
				Tag *tag = 0;
				if (grammar->single_tags.find(*tter) != grammar->single_tags.end()) {
					tag = grammar->single_tags.find(*tter)->second;
				}
				else {
					tag = single_tags.find(*tter)->second;
				}
				assert(tag != 0);
				if (tag->type & T_MAPPING) {
					swindow->tags_mapped[*tter] = *tter;
				}
				if (tag->type & T_TEXTUAL) {
					swindow->tags_textual[*tter] = *tter;
				}
				if (!tag->features && !tag->type) {
					swindow->tags_plain[*tter] = *tter;
				}
			}
		}
	}
	swindow->rehash();
}

inline void GrammarApplicator::reflowReading(Reading *reading) {
	reading->tags.clear();
	reading->tags_mapped.clear();
	reading->tags_plain.clear();
	reading->tags_textual.clear();

	std::list<uint32_t>::const_iterator tter;
	for (tter = reading->tags_list.begin() ; tter != reading->tags_list.end() ; tter++) {
		reading->tags[*tter] = *tter;
		Tag *tag = 0;
		if (grammar->single_tags.find(*tter) != grammar->single_tags.end()) {
			tag = grammar->single_tags.find(*tter)->second;
		}
		else {
			tag = single_tags.find(*tter)->second;
		}
		assert(tag != 0);
		if (tag->type & T_MAPPING) {
			reading->tags_mapped[*tter] = *tter;
		}
		if (tag->type & T_TEXTUAL) {
			reading->tags_textual[*tter] = *tter;
		}
		if (!tag->features && !tag->type) {
			reading->tags_plain[*tter] = *tter;
		}
	}
	reading->rehash();
}

int GrammarApplicator::runGrammarOnText(UFILE *input, UFILE *output) {
	if (!input) {
		u_fprintf(ux_stderr, "Error: Input is null - nothing to parse!\n");
		return -1;
	}
	u_frewind(input);
	if (u_feof(input)) {
		u_fprintf(ux_stderr, "Error: Input is empty - nothing to parse!\n");
		return -1;
	}
	if (!output) {
		u_fprintf(ux_stderr, "Error: Output is null - cannot write to nothing!\n");
		return -1;
	}
	if (!grammar) {
		u_fprintf(ux_stderr, "Error: No grammar provided - cannot continue! Hint: call setGrammar() first.\n");
		return -1;
	}

	free_strings();
	free_keywords();
	int error = init_keywords();
	if (error) {
		u_fprintf(ux_stderr, "Error: init_keywords returned %u!\n", error);
		return error;
	}
	error = init_strings();
	if (error) {
		u_fprintf(ux_stderr, "Error: init_strings returned %u!\n", error);
		return error;
	}

#define BUFFER_SIZE (131072)
	UChar _line[BUFFER_SIZE];
	UChar *line = _line;
	UChar _cleaned[BUFFER_SIZE];
	UChar *cleaned = _cleaned;

	begintag = addTag(stringbits[S_BEGINTAG]);
	endtag = addTag(stringbits[S_ENDTAG]);

	uint32_t lines = 0;
	Window *cWindow = new Window();
	SingleWindow *cSWindow = 0;
	Cohort *cCohort = 0;
	Reading *cReading = 0;

	SingleWindow *lSWindow = 0;
	Cohort *lCohort = 0;
	Reading *lReading = 0;

	cWindow->num_windows = num_windows;

	while (!u_feof(input)) {
		u_fgets(line, BUFFER_SIZE-1, input);
		u_strcpy(cleaned, line);
		ux_packWhitespace(cleaned);

		if (cleaned[0] == '"' && cleaned[1] == '<') {
			ux_trim(cleaned);
			if (cCohort && doesTagMatchSet(cCohort->wordform, grammar->delimiters->hash)) {
				if (cCohort->readings.empty()) {
					cReading = new Reading();
					cReading->wordform = cCohort->wordform;
					cReading->baseform = cCohort->wordform;
					cReading->tags[cCohort->wordform] = cCohort->wordform;
					cReading->tags_list.push_back(cCohort->wordform);
					cReading->noprint = true;
					cReading->rehash();
					cCohort->readings.push_back(cReading);
					lReading = cReading;
				}
				std::list<Reading*>::iterator iter;
				for (iter = cCohort->readings.begin() ; iter != cCohort->readings.end() ; iter++) {
					(*iter)->tags_list.push_back(endtag);
					(*iter)->tags[endtag] = endtag;
					(*iter)->rehash();
				}

				cSWindow->cohorts.push_back(cCohort);
				cWindow->next.push_back(cSWindow);
				lSWindow = cSWindow;
				lCohort = cCohort;
				cSWindow = 0;
				cCohort = 0;
			}
			if (!cSWindow) {
				cReading = new Reading();
				cReading->baseform = begintag;
				cReading->wordform = begintag;
				cReading->tags[begintag] = begintag;
				cReading->tags_list.push_back(begintag);
				cReading->rehash();

				cCohort = new Cohort();
				cCohort->wordform = begintag;
				cCohort->readings.push_back(cReading);

				cSWindow = new SingleWindow();
				cSWindow->cohorts.push_back(cCohort);

				lSWindow = cSWindow;
				lReading = cReading;
				lCohort = cCohort;
				cCohort = 0;
			}
			if (cCohort && cSWindow) {
				cSWindow->cohorts.push_back(cCohort);
				lCohort = cCohort;
				if (cCohort->readings.empty()) {
					cReading = new Reading();
					cReading->wordform = cCohort->wordform;
					cReading->baseform = cCohort->wordform;
					cReading->tags[cCohort->wordform] = cCohort->wordform;
					cReading->noprint = true;
					cReading->rehash();
					cCohort->readings.push_back(cReading);
					lReading = cReading;
				}
			}
			if (cWindow->next.size() > num_windows) {
				cWindow->shuffleWindowsDown();
				runGrammarOnWindow(cWindow);
				printSingleWindow(cWindow->current, output);
			}
			cCohort = new Cohort();
			cCohort->wordform = addTag(cleaned);
			lCohort = cCohort;
			lReading = 0;
		}
		else if (cleaned[0] == ' ' && cleaned[1] == '"' && cCohort) {
			cReading = new Reading();
			cReading->wordform = cCohort->wordform;
			cReading->tags[cReading->wordform] = cReading->wordform;

			ux_trim(cleaned);
			UChar *space = cleaned;
			UChar *base = space;

			while (space && (space = u_strchr(space, ' ')) != 0) {
				space[0] = 0;
				space++;
				if (u_strlen(base)) {
					uint32_t tag = addTag(base);
					if (!cReading->baseform && single_tags[tag]->type & T_BASEFORM) {
						cReading->baseform = tag;
					}
					cReading->tags_list.push_back(tag);
				}
				base = space;
			}
			if (u_strlen(base)) {
				uint32_t tag = addTag(base);
				if (!cReading->baseform && single_tags[tag]->type & T_BASEFORM) {
					cReading->baseform = tag;
				}
				cReading->tags_list.push_back(tag);
			}
			reflowReading(cReading);
			if (!cReading->tags_mapped.empty()) {
				cReading->mapped = true;
			}
			cCohort->readings.push_back(cReading);
			lReading = cReading;
		}
		else {
			if (cleaned[0] == ' ' && cleaned[1] == '"') {
				u_fprintf(ux_stderr, "Warning: Line %u looked like a reading but there was no containing cohort - treated as plain text.\n", lines);
			}
			ux_trim(cleaned);
			if (u_strlen(cleaned) > 0) {
				if (lReading) {
					lReading->text = ux_append(lReading->text, line);
				}
				else if (lCohort) {
					lCohort->text = ux_append(lCohort->text, line);
				}
				else if (lSWindow) {
					lSWindow->text = ux_append(lSWindow->text, line);
				}
				else {
					u_fprintf(output, "%S", line);
				}
			}
		}
		lines++;
	}

	if (cCohort && cSWindow) {
		cSWindow->cohorts.push_back(cCohort);
		lCohort = cCohort;
		if (cCohort->readings.empty()) {
			cReading = new Reading();
			cReading->wordform = cCohort->wordform;
			cReading->baseform = cCohort->wordform;
			cReading->tags[cCohort->wordform] = cCohort->wordform;
			cReading->noprint = true;
			cReading->rehash();
			cCohort->readings.push_back(cReading);
			lReading = cReading;
		}
		cWindow->next.push_back(cSWindow);
	}
	while (!cWindow->next.empty()) {
		cWindow->shuffleWindowsDown();
		runGrammarOnWindow(cWindow);
		printSingleWindow(cWindow->current, output);
	}
	u_fflush(output);
	std::cerr << "Cache " << (cache_hits+cache_miss) << " : " << cache_hits << " / " << cache_miss << std::endl;
	std::cerr << "Match " << (match_sub+match_comp+match_single) << " : " << match_sub << " / " << match_comp << " / " << match_single << std::endl;
	return 0;
}

int GrammarApplicator::runGrammarOnWindow(Window *window) {
label_runGrammarOnWindow_begin:
	SingleWindow *current = window->current;

	if ((apply_mappings || apply_corrections) && !grammar->mappings.empty()) {
		for (uint32_t j=0;j<grammar->mappings.size();j++) {
			const Rule *rule = grammar->mappings[j];

			if (!apply_mappings && (rule->type == K_MAP || rule->type == K_ADD || rule->type == K_REPLACE)) {
				continue;
			}
			if (!apply_corrections && (rule->type == K_SUBSTITUTE || rule->type == K_APPEND)) {
				continue;
			}

			for (uint32_t c=0 ; c < current->cohorts.size() ; c++) {
				if (c == 0) {
					continue;
				}
				Cohort *cohort = current->cohorts[c];

				std::list<Reading*>::iterator rter;
				for (rter = cohort->readings.begin() ; rter != cohort->readings.end() ; rter++) {
					Reading *reading = *rter;
					if (reading->mapped) {
						continue;
					}
					if (!reading->hash) {
						reading->rehash();
					}
					if (!rule->wordform || rule->wordform == reading->wordform) {
						if (rule->target && doesSetMatchReading(reading, rule->target)) {
							bool good = true;
							if (!rule->tests.empty()) {
								std::list<ContextualTest*>::iterator iter;
								for (iter = rule->tests.begin() ; iter != rule->tests.end() ; iter++) {
									ContextualTest *test = *iter;
									good = runContextualTest(window, current, c, test);
									if (!good) {
										if (test != rule->tests.front()) {
											rule->tests.remove(test);
											rule->tests.push_front(test);
										}
										break;
									}
								}
							}
							if (good) {
								reading->mapped_by.push_back(j);
								if (rule->type == K_ADD || rule->type == K_MAP) {
									std::list<uint32_t>::const_iterator tter;
									for (tter = rule->maplist.begin() ; tter != rule->maplist.end() ; tter++) {
										reading->tags_list.push_back(*tter);
									}
									reflowReading(reading);
								}
								if (rule->type == K_REPLACE) {
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
								if (rule->type == K_SUBSTITUTE) {
									std::list<uint32_t>::const_iterator tter;
									for (tter = rule->sublist.begin() ; tter != rule->sublist.end() ; tter++) {
										reading->tags_list.remove(*tter);
									}
									for (tter = rule->maplist.begin() ; tter != rule->maplist.end() ; tter++) {
										reading->tags_list.push_back(*tter);
									}
									reflowReading(reading);
									if (!reading->tags_mapped.empty()) {
										reading->mapped = true;
									}
								}
								// ToDo: Implement APPEND
								if (rule->type == K_MAP) {
									reading->mapped = true;
								}
							}
						}
					}
				}
			}
		}
	}

	if (!grammar->rules.empty()) {
		reflowSingleWindow(current);
		
		for (uint32_t i=0;i<grammar->sections.size()-1;) {
			bool section_did_good = false;
			bool select_only = reorder;
			for (uint32_t j=0;j<grammar->sections[i+1];) {
				const Rule *rule = grammar->rules[j];
				const Rule *removerule = 0;
				const Rule *selectrule = 0;
				Reading *deleted = 0;
				Reading *selected = 0;
				KEYWORDS type = rule->type;

				for (uint32_t c=0 ; c < current->cohorts.size() ; c++) {
					if (c == 0) {
						continue;
					}
					if (select_only && rule->type != K_SELECT) {
						break;
					}
					Cohort *cohort = current->cohorts[c];
					if (cohort->readings.empty()) {
						continue;
					}
					if ((type == K_SELECT || type == K_REMOVE || type == K_IFF) && cohort->readings.size() <= 1 && cohort->readings.front()->tags_mapped.size() <= 1) {
						continue;
					}
					if (type == K_DELIMIT && c == current->cohorts.size()-1) {
						continue;
					}
					if (rule->wordform && rule->wordform != cohort->wordform) {
						rule->num_fail++;
						continue;
					}

					std::list<Reading*>::iterator rter;
					for (rter = cohort->readings.begin() ; rter != cohort->readings.end() ; rter++) {
						Reading *reading = *rter;
						if (!reading->hash) {
							reading->rehash();
						}
						if (rule->target && doesSetMatchReading(reading, rule->target)) {
							bool good = true;
							if (!rule->tests.empty()) {
								bool test_good = false;
								std::list<ContextualTest*>::iterator iter;
								for (iter = rule->tests.begin() ; iter != rule->tests.end() ; iter++) {
									ContextualTest *test = *iter;
									test_good = runContextualTest(window, current, c, test);
									if (!test_good) {
										good = test_good;
										if (!statistics) {
											/*
											if (test != rule->tests.front()) {
												rule->tests.remove(test);
												rule->tests.push_front(test);
											}
											//*/
											break;
										}
									}
								}
							}
							if (rule->type == K_IFF && good) {
								type = K_SELECT;
								good = true;
							}
							else if (rule->type == K_IFF && !good) {
								type = K_REMOVE;
								good = true;
							}
							if (good) {
								rule->num_match++;
								reading->hit_by.push_back(j);
								if (type == K_REMOVE) {
									removerule = rule;
									reading->deleted = true;
									cohort->deleted.push_back(reading);
									cohort->readings.remove(reading);
									deleted = reading;
									std::list<Reading*> removed;
									for (rter = cohort->readings.begin() ; rter != cohort->readings.end() ; rter++) {
										Reading *reading = *rter;
										if (deleted != reading && doesSetMatchReading(reading, removerule->target)) {
											reading->hit_by.push_back(deleted->hit_by.back());
											reading->deleted = true;
											removed.push_back(reading);
											cohort->deleted.push_back(reading);
											cohort->readings.remove(reading);
											rter = cohort->readings.begin();
											rter--;
										}
									}
									if (cohort->readings.empty()) {
										for (rter = removed.begin() ; rter != removed.end() ; rter++) {
											Reading *reading = *rter;
											reading->deleted = false;
											cohort->readings.push_back(reading);
											cohort->deleted.remove(reading);
										}
										good = false;
									}
									removed.clear();
									break;
								}
								else if (type == K_SELECT) {
									selectrule = rule;
									reading->selected = true;
									selected = reading;
									size_t nc = cohort->readings.size();
									for (rter = cohort->readings.begin() ; rter != cohort->readings.end() ; rter++) {
										Reading *reading = *rter;
										if (selected != reading) {
											reading->hit_by.push_back(selected->hit_by.back());
											if (doesSetMatchReading(reading, selectrule->target)) {
												reading->selected = true;
											} else {
												reading->deleted = true;
												cohort->deleted.push_back(reading);
												cohort->readings.remove(reading);
												rter = cohort->readings.begin();
												rter--;
											}
										}
									}
									// This SELECT had no effect, so don't mark section as active.
									if (nc == cohort->readings.size()) {
										good = false;
									}
									break;
								}
								else if (type == K_REMVARIABLE) {
									variables[rule->varname] = 0;
									good = false;
								}
								else if (type == K_SETVARIABLE) {
									variables[rule->varname] = rule->varvalue;
									good = false;
								}
								else if (type == K_DELIMIT) {
									SingleWindow *nwin = new SingleWindow();
									uint32_t nc = c+1;

									Reading *cReading = new Reading();
									cReading->baseform = begintag;
									cReading->wordform = begintag;
									cReading->tags[begintag] = begintag;
									cReading->tags_list.push_back(begintag);
									cReading->rehash();

									Cohort *cCohort = new Cohort();
									cCohort->wordform = begintag;
									cCohort->readings.push_back(cReading);

									nwin->cohorts.push_back(cCohort);

									for ( ; nc < current->cohorts.size() ; nc++) {
										nwin->cohorts.push_back(current->cohorts.at(nc));
									}
									c = (uint32_t)current->cohorts.size()-c;
									for (nc = 0 ; nc < c-1 ; nc++) {
										current->cohorts.pop_back();
									}
									window->next.push_front(nwin);

									cohort = current->cohorts.back();
									for (rter = cohort->readings.begin() ; rter != cohort->readings.end() ; rter++) {
										Reading *reading = *rter;
										reading->tags_list.push_back(endtag);
										reading->tags[endtag] = endtag;
										reading->rehash();
									}
									goto label_runGrammarOnWindow_begin;
								}
								if (good) {
									section_did_good = true;
								}
							}
							else {
								rule->num_fail++;
							}
						}
						else {
							rule->num_fail++;
						}
					}
				}
				if (j == grammar->sections[i+1]) {
					if (single_run) {
						section_did_good = false;
					}
					if (select_only) {
						select_only = false;
						j = 0;
						continue;
					}
					else if (!section_did_good && i < grammar->sections.size()-1) {
						i++;
					}
				}
				j++;
			}
			if (!section_did_good) {
				i++;
			}
		}
	}
	return 0;
}

bool GrammarApplicator::runContextualTest(const Window *window, const SingleWindow *sWindow, const uint32_t position, const ContextualTest *test) {
	bool retval = true;
	int pos = position + test->offset;
	const Cohort *cohort = 0;
	// ToDo: (NOT *) and (*C) tests can be cached
	if (test->absolute) {
		if (test->offset < 0) {
			pos = ((int)sWindow->cohorts.size()-1) - test->offset;
		}
		else {
			pos = test->offset;
		}
	}
	if (pos >= 0 && (uint32_t)pos < sWindow->cohorts.size()) {
		cohort = sWindow->cohorts.at(pos);
	}
	if (!cohort) {
		retval = false;
	}
	else {
		bool foundfirst = false;
		if (test->offset < 0 && pos >= 0 && (test->scanall || test->scanfirst)) {
			for (int i=pos;i>=0;i--) {
				cohort = sWindow->cohorts.at(i);
				retval = doesSetMatchCohortNormal(cohort, test->target);
				foundfirst = retval;
				if (test->careful) {
					if (retval) {
						retval = doesSetMatchCohortCareful(cohort, test->target);
					}
					else {
						retval = false;
					}
				}
				if (test->negative) {
					retval = !retval;
				}
				if (retval && test->linked) {
					retval = runContextualTest(window, sWindow, i, test->linked);
				}
				if (foundfirst && test->scanfirst) {
					break;
				}
				if (test->barrier) {
					bool barrier = doesSetMatchCohortNormal(cohort, test->barrier);
					if (barrier) {
						break;
					}
				}
			}
		}
		else if (test->offset > 0 && (uint32_t)pos <= sWindow->cohorts.size() && (test->scanall || test->scanfirst)) {
			for (uint32_t i=pos;i<sWindow->cohorts.size();i++) {
				cohort = sWindow->cohorts.at(i);
				retval = doesSetMatchCohortNormal(cohort, test->target);
				foundfirst = retval;
				if (test->careful) {
					if (retval) {
						retval = doesSetMatchCohortCareful(cohort, test->target);
					}
					else {
						retval = false;
					}
				}
				if (test->negative) {
					retval = !retval;
				}
				if (retval && test->linked) {
					retval = runContextualTest(window, sWindow, i, test->linked);
				}
				if (foundfirst && test->scanfirst) {
					break;
				}
				if (test->barrier) {
					bool barrier = doesSetMatchCohortNormal(cohort, test->barrier);
					if (barrier) {
						break;
					}
				}
			}
		}
		else {
			if (test->careful) {
				retval = doesSetMatchCohortCareful(cohort, test->target);
			}
			else {
				retval = doesSetMatchCohortNormal(cohort, test->target);
			}
			if (test->negative) {
				retval = !retval;
			}
			if (retval && test->linked) {
				retval = runContextualTest(window, sWindow, pos, test->linked);
			}
		}
	}
	if (retval) {
		test->num_match++;
	} else {
		test->num_fail++;
	}
	return retval;
}

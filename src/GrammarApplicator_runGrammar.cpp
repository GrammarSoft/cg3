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

	uint32_t begintag = addTag(stringbits[S_BEGINTAG]);
	uint32_t endtag = addTag(stringbits[S_ENDTAG]);

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
					cReading->rehash();
					cReading->noprint = true;
					cCohort->readings.push_back(cReading);
					lReading = cReading;
				}
				std::vector<Reading*>::iterator iter;
				for (iter = cCohort->readings.begin() ; iter != cCohort->readings.end() ; iter++) {
					(*iter)->tags[endtag] = endtag;
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
					cReading->rehash();
					cReading->noprint = true;
					cCohort->readings.push_back(cReading);
					lReading = cReading;
				}
			}
			if (cWindow->next.size() > num_windows) {
				cWindow->shuffleWindowsDown();
				runGrammarOnWindow(cWindow);
				printSingleWindow(cWindow->current, output);
				std::cerr << "Cache " << cache_hits << " / " << cache_miss << "\r" << std::flush;
				u_fflush(output);
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
					if (single_tags[tag]->type & T_MAPPING) {
						cReading->mapped = true;
					}
					cReading->tags[tag] = tag;
					cReading->tags_list.push_back(tag);
				}
				base = space;
			}
			if (u_strlen(base)) {
				uint32_t tag = addTag(base);
				if (!cReading->baseform && single_tags[tag]->type & T_BASEFORM) {
					cReading->baseform = tag;
				}
				if (single_tags[tag]->type & T_MAPPING) {
					cReading->mapped = true;
				}
				cReading->tags[tag] = tag;
				cReading->tags_list.push_back(tag);
			}
			cCohort->readings.push_back(cReading);
			lReading = cReading;
		}
		else {
			if (cleaned[0] == ' ' && cleaned[1] == '"') {
				u_fprintf(ux_stderr, "Warning: Line %u looked like a reading but there was no containing cohort - treated as plain text.\n", lines);
			}
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
		lines++;
	}

	while (!cWindow->next.empty()) {
		cWindow->shuffleWindowsDown();
		runGrammarOnWindow(cWindow);
		printSingleWindow(cWindow->current, output);
		u_fflush(output);
	}
	std::cerr << "Cache " << cache_hits << " / " << cache_miss << std::endl;
	return 0;
}

int GrammarApplicator::runGrammarOnWindow(Window *window) {
	SingleWindow *current = window->current;

	for (uint32_t c=0 ; c < current->cohorts.size() ; c++) {
		if (c == 0) {
			continue;
		}
		Cohort *cohort = current->cohorts[c];
		const Rule *selectrule = 0;
		Reading *selected = 0;
		
		std::vector<Reading*>::iterator rter;
		for (rter = cohort->readings.begin() ; rter != cohort->readings.end() ; rter++) {
			Reading *reading = *rter;
			reading->rehash();
			if (selected && selected != reading && !doesSetMatchReading(reading, selectrule->target)) {
				reading->deleted = true;
				reading->hit_by = selected->hit_by;
			}
			if (!reading->deleted) {
				for (uint32_t i=0;i<grammar->sections.size();i++) {
					for (uint32_t j=0;j<grammar->sections[i];j++) {
						const Rule *rule = grammar->rules[j];
						if (!rule->wordform || rule->wordform == reading->wordform) {
							if (rule->target && doesSetMatchReading(reading, rule->target)) {
								bool good = true;
								if (!rule->tests.empty()) {
									std::list<ContextualTest*>::const_iterator iter;
									for (iter = rule->tests.begin() ; iter != rule->tests.end() ; iter++) {
										const ContextualTest *test = *iter;
										good = runContextualTest(window, current, c, test);
										if (!good) {
											break;
										}
									}
								}
								if (good) {
									if (rule->type == K_REMOVE) {
										reading->deleted = true;
										reading->hit_by = j;
										break;
									}
									else if (rule->type == K_SELECT) {
										selectrule = rule;
										reading->selected = true;
										reading->hit_by = j;
										selected = reading;
										break;
									}
								}
							}
						}
					}
					if (reading->deleted || reading->selected || selected) {
						break;
					}
				}
			}
		}
	}
	return 0;
}

bool GrammarApplicator::runContextualTest(const Window *window, const SingleWindow *sWindow, const uint32_t position, const ContextualTest *test) {
	bool retval = true;
	int pos = position + test->offset;
	const Cohort *cohort = 0;
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
				cohort = sWindow->cohorts.at(pos);
				if (test->careful) {
					retval = doesSetMatchCohortCareful(cohort, test->target);
				}
				else {
					retval = doesSetMatchCohortNormal(cohort, test->target);
				}
				foundfirst = retval;
				if (test->negative) {
					retval = !retval;
				}
				if (!retval && foundfirst && test->scanfirst) {
					break;
				}
				else if (test->barrier) {
					bool barrier = doesSetMatchCohortNormal(cohort, test->barrier);
					if (barrier) {
						break;
					}
				}
				if (retval && test->linked) {
					retval = runContextualTest(window, sWindow, i, test->linked);
				}
			}
		}
		else if (test->offset > 0 && (uint32_t)pos <= sWindow->cohorts.size() && (test->scanall || test->scanfirst)) {
			for (uint32_t i=pos;i<sWindow->cohorts.size();i++) {
				cohort = sWindow->cohorts.at(pos);
				if (test->careful) {
					retval = doesSetMatchCohortCareful(cohort, test->target);
				}
				else {
					retval = doesSetMatchCohortNormal(cohort, test->target);
				}
				foundfirst = retval;
				if (test->negative) {
					retval = !retval;
				}
				if (!retval && foundfirst && test->scanfirst) {
					break;
				}
				else if (test->barrier) {
					bool barrier = doesSetMatchCohortNormal(cohort, test->barrier);
					if (barrier) {
						break;
					}
				}
				if (retval && test->linked) {
					retval = runContextualTest(window, sWindow, i, test->linked);
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
	return retval;
}

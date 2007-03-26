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

	// ToDo: Add flag for using dependencies to delimit windows
	if (!grammar->delimiters || (grammar->delimiters->sets.empty() && grammar->delimiters->tags_map.empty())) {
		if (!grammar->soft_delimiters || (grammar->soft_delimiters->sets.empty() && grammar->soft_delimiters->tags_map.empty())) {
			u_fprintf(ux_stderr, "Warning: No soft or hard delimiters defined in grammar. Hard limit of %u cohorts may break windows in unintended places.\n", hard_limit);
		}
		else {
			u_fprintf(ux_stderr, "Warning: No hard delimiters defined in grammar. Soft limit of %u cohorts may break windows in unintended places.\n", soft_limit);
		}
	}

#define BUFFER_SIZE (131072)
	UChar _line[BUFFER_SIZE];
	UChar *line = _line;
	UChar _cleaned[BUFFER_SIZE];
	UChar *cleaned = _cleaned;
	bool ignoreinput = false;

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

	cWindow->parent = this;
	cWindow->window_span = num_windows;

	while (!u_feof(input)) {
		u_fgets(line, BUFFER_SIZE-1, input);
		u_strcpy(cleaned, line);
		ux_packWhitespace(cleaned);

		if (!ignoreinput && cleaned[0] == '"' && cleaned[1] == '<') {
			ux_trim(cleaned);
			if (cCohort && cSWindow->cohorts.size() >= soft_limit && doesTagMatchSet(cCohort->wordform, grammar->soft_delimiters->hash)) {
				if (cSWindow->cohorts.size() >= soft_limit) {
					u_fprintf(ux_stderr, "Warning: Soft limit of %u cohorts reached but found suitable soft delimiter.\n", soft_limit);
				}
				if (cCohort->readings.empty()) {
					cReading = new Reading();
					cReading->wordform = cCohort->wordform;
					cReading->baseform = cCohort->wordform;
					cReading->tags[cCohort->wordform] = cCohort->wordform;
					cReading->tags_list.push_back(cCohort->wordform);
					cReading->noprint = true;
					reflowReading(cReading);
					cCohort->appendReading(cReading);
					lReading = cReading;
				}
				std::list<Reading*>::iterator iter;
				for (iter = cCohort->readings.begin() ; iter != cCohort->readings.end() ; iter++) {
					(*iter)->tags_list.push_back(endtag);
					(*iter)->tags[endtag] = endtag;
					(*iter)->rehash();
				}

				cSWindow->appendCohort(cCohort);
				cWindow->appendSingleWindow(cSWindow);
				lSWindow = cSWindow;
				lCohort = cCohort;
				cSWindow = 0;
				cCohort = 0;
			}
			if (cCohort && (cSWindow->cohorts.size() >= hard_limit || doesTagMatchSet(cCohort->wordform, grammar->delimiters->hash))) {
				if (cSWindow->cohorts.size() >= hard_limit) {
					u_fprintf(ux_stderr, "Warning: Hard limit of %u cohorts reached - forcing break.\n", hard_limit);
				}
				if (cCohort->readings.empty()) {
					cReading = new Reading();
					cReading->wordform = cCohort->wordform;
					cReading->baseform = cCohort->wordform;
					cReading->tags[cCohort->wordform] = cCohort->wordform;
					cReading->tags_list.push_back(cCohort->wordform);
					cReading->noprint = true;
					reflowReading(cReading);
					cCohort->appendReading(cReading);
					lReading = cReading;
				}
				std::list<Reading*>::iterator iter;
				for (iter = cCohort->readings.begin() ; iter != cCohort->readings.end() ; iter++) {
					(*iter)->tags_list.push_back(endtag);
					(*iter)->tags[endtag] = endtag;
					(*iter)->rehash();
				}

				cSWindow->appendCohort(cCohort);
				cWindow->appendSingleWindow(cSWindow);
				lSWindow = cSWindow;
				lCohort = cCohort;
				cSWindow = 0;
				cCohort = 0;
			}
			if (!cSWindow) {
				// ToDo: Refactor to allocate SingleWindow, Cohort, and Reading from their containers
				cSWindow = new SingleWindow(cWindow);

				cReading = new Reading();
				cReading->baseform = begintag;
				cReading->wordform = begintag;
				cReading->tags[begintag] = begintag;
				cReading->tags_list.push_back(begintag);
				cReading->rehash();

				cCohort = new Cohort(cSWindow);
				cCohort->global_number = cWindow->cohort_counter++;
				cCohort->wordform = begintag;
				cCohort->appendReading(cReading);

				cSWindow->appendCohort(cCohort);

				lSWindow = cSWindow;
				lReading = cReading;
				lCohort = cCohort;
				cCohort = 0;
			}
			if (cCohort && cSWindow) {
				cSWindow->appendCohort(cCohort);
				lCohort = cCohort;
				if (cCohort->readings.empty()) {
					cReading = new Reading();
					cReading->wordform = cCohort->wordform;
					cReading->baseform = cCohort->wordform;
					cReading->tags[cCohort->wordform] = cCohort->wordform;
					cReading->tags_list.push_back(cCohort->wordform);
					cReading->noprint = true;
					reflowReading(cReading);
					cCohort->appendReading(cReading);
					lReading = cReading;
				}
			}
			if (cWindow->next.size() > num_windows) {
				cWindow->shuffleWindowsDown();
				runGrammarOnWindow(cWindow);
				printSingleWindow(cWindow->current, output);
			}
			cCohort = new Cohort(cSWindow);
			cCohort->global_number = cWindow->cohort_counter++;
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
			cCohort->appendReading(cReading);
			lReading = cReading;
		}
		else {
			if (!ignoreinput && cleaned[0] == ' ' && cleaned[1] == '"') {
				u_fprintf(ux_stderr, "Warning: Line %u looked like a reading but there was no containing cohort - treated as plain text.\n", lines);
			}
			ux_trim(cleaned);
			if (u_strlen(cleaned) > 0) {
				if (u_strcmp(cleaned, stringbits[S_CMD_FLUSH]) == 0) {
					u_fprintf(ux_stderr, "Info: CGCMD:FLUSH encountered on line %u. Flushing...\n", lines);
					if (cCohort && cSWindow) {
						cSWindow->appendCohort(cCohort);
						if (cCohort->readings.empty()) {
							cReading = new Reading();
							cReading->wordform = cCohort->wordform;
							cReading->baseform = cCohort->wordform;
							cReading->tags[cCohort->wordform] = cCohort->wordform;
							cReading->noprint = true;
							cReading->rehash();
							cCohort->appendReading(cReading);
						}
						std::list<Reading*>::iterator iter;
						for (iter = cCohort->readings.begin() ; iter != cCohort->readings.end() ; iter++) {
							(*iter)->tags_list.push_back(endtag);
							(*iter)->tags[endtag] = endtag;
							(*iter)->rehash();
						}
						cWindow->appendSingleWindow(cSWindow);
						cReading = lReading = 0;
						cCohort = lCohort = 0;
						cSWindow = lSWindow = 0;
					}
					while (!cWindow->next.empty()) {
						cWindow->shuffleWindowsDown();
						runGrammarOnWindow(cWindow);
						printSingleWindow(cWindow->current, output);
					}
					u_fflush(output);
				}
				else if (u_strcmp(cleaned, stringbits[S_CMD_IGNORE]) == 0) {
					u_fprintf(ux_stderr, "Info: CGCMD:IGNORE encountered on line %u. Passing through all input...\n", lines);
					ignoreinput = true;
				}
				else if (u_strcmp(cleaned, stringbits[S_CMD_RESUME]) == 0) {
					u_fprintf(ux_stderr, "Info: CGCMD:RESUME encountered on line %u. Resuming CG...\n", lines);
					ignoreinput = false;
				}
				else if (u_strcmp(cleaned, stringbits[S_CMD_EXIT]) == 0) {
					u_fprintf(ux_stderr, "Info: CGCMD:EXIT encountered on line %u. Exiting...\n", lines);
					u_fprintf(output, "%S", line);
					goto CGCMD_EXIT;
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
		}
		lines++;
	}

	if (cCohort && cSWindow) {
		cSWindow->appendCohort(cCohort);
		if (cCohort->readings.empty()) {
			cReading = new Reading();
			cReading->wordform = cCohort->wordform;
			cReading->baseform = cCohort->wordform;
			cReading->tags[cCohort->wordform] = cCohort->wordform;
			cReading->noprint = true;
			cReading->rehash();
			cCohort->appendReading(cReading);
		}
		std::list<Reading*>::iterator iter;
		for (iter = cCohort->readings.begin() ; iter != cCohort->readings.end() ; iter++) {
			(*iter)->tags_list.push_back(endtag);
			(*iter)->tags[endtag] = endtag;
			(*iter)->rehash();
		}
		cWindow->appendSingleWindow(cSWindow);
		cReading = 0;
		cCohort = 0;
		cSWindow = 0;
	}
	while (!cWindow->next.empty()) {
		cWindow->shuffleWindowsDown();
		runGrammarOnWindow(cWindow);
		printSingleWindow(cWindow->current, output);
	}

	u_fflush(output);

CGCMD_EXIT:
	std::cerr << "Cache " << (cache_hits+cache_miss) << " : " << cache_hits << " / " << cache_miss << std::endl;
	std::cerr << "Match " << (match_sub+match_comp+match_single) << " : " << match_sub << " / " << match_comp << " / " << match_single << std::endl;

	delete cWindow;
	return 0;
}

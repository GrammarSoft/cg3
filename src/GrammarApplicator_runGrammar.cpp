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
#include "Strings.h"
#include "Tag.h"
#include "Grammar.h"
#include "Window.h"
#include "SingleWindow.h"
#include "Reading.h"
#include "Recycler.h"

using namespace CG3;
using namespace CG3::Strings;

Reading *GrammarApplicator::initEmptyCohort(Cohort *cCohort) {
	Recycler *r = Recycler::instance();
	Reading *cReading = r->new_Reading(cCohort);
	cReading->wordform = cCohort->wordform;
	cReading->baseform = cCohort->wordform;
	if (grammar->sets_any && !grammar->sets_any->empty()) {
		cReading->parent->possible_sets.insert(grammar->sets_any->begin(), grammar->sets_any->end());
		cReading->possible_sets.insert(grammar->sets_any->begin(), grammar->sets_any->end());
	}
	addTagToReading(cReading, cCohort->wordform);
	cReading->noprint = true;
	cCohort->appendReading(cReading);
	numReadings++;
	return cReading;
}

int GrammarApplicator::runGrammarOnText(UFILE *input, UFILE *output) {
	if (!input) {
		u_fprintf(ux_stderr, "Error: Input is null - nothing to parse!\n");
		CG3Quit(1);
	}
	u_frewind(input);
	if (u_feof(input)) {
		u_fprintf(ux_stderr, "Error: Input is empty - nothing to parse!\n");
		CG3Quit(1);
	}
	if (!output) {
		u_fprintf(ux_stderr, "Error: Output is null - cannot write to nothing!\n");
		CG3Quit(1);
	}
	if (!grammar) {
		u_fprintf(ux_stderr, "Error: No grammar provided - cannot continue! Hint: call setGrammar() first.\n");
		CG3Quit(1);
	}

	if (!grammar->delimiters || (grammar->delimiters->sets.empty() && grammar->delimiters->tags_set.empty())) {
		if (!grammar->soft_delimiters || (grammar->soft_delimiters->sets.empty() && grammar->soft_delimiters->tags_set.empty())) {
			u_fprintf(ux_stderr, "Warning: No soft or hard delimiters defined in grammar. Hard limit of %u cohorts may break windows in unintended places.\n", hard_limit);
		}
		else {
			u_fprintf(ux_stderr, "Warning: No hard delimiters defined in grammar. Soft limit of %u cohorts may break windows in unintended places.\n", soft_limit);
		}
	}

	UChar _line[CG3_BUFFER_SIZE];
	UChar *line = _line;
	UChar _cleaned[CG3_BUFFER_SIZE];
	UChar *cleaned = _cleaned;
	bool ignoreinput = false;

	index();

	Recycler *r = Recycler::instance();
	uint32_t resetAfter = ((num_windows+4)*2+1);
	uint32_t lines = 0;

	begintag = addTag(stringbits[S_BEGINTAG])->hash;
	endtag = addTag(stringbits[S_ENDTAG])->hash;

	SingleWindow *cSWindow = 0;
	Cohort *cCohort = 0;
	Reading *cReading = 0;

	SingleWindow *lSWindow = 0;
	Cohort *lCohort = 0;
	Reading *lReading = 0;

	gWindow->window_span = num_windows;
	gtimer = getticks();
	ticks timer(gtimer);

	while (!u_feof(input)) {
		lines++;
		u_fgets(line, CG3_BUFFER_SIZE-1, input);
		u_strcpy(cleaned, line);
		ux_packWhitespace(cleaned);

		if (!ignoreinput && cleaned[0] == '"' && cleaned[1] == '<') {
			ux_trim(cleaned);
			if (cCohort && cSWindow->cohorts.size() >= soft_limit && grammar->soft_delimiters && doesTagMatchSet(cCohort->wordform, grammar->soft_delimiters)) {
				if (cSWindow->cohorts.size() >= soft_limit) {
					if (verbosity_level > 0) {
						u_fprintf(ux_stderr, "Warning: Soft limit of %u cohorts reached at line %u but found suitable soft delimiter.\n", soft_limit, numLines);
						u_fflush(ux_stderr);
					}
				}
				if (cCohort->readings.empty()) {
					cReading = initEmptyCohort(cCohort);
					lReading = cReading;
				}
				foreach (std::list<Reading*>, cCohort->readings, iter, iter_end) {
					addTagToReading(*iter, endtag);
				}

				cSWindow->appendCohort(cCohort);
				gWindow->appendSingleWindow(cSWindow);
				lSWindow = cSWindow;
				lCohort = cCohort;
				cSWindow = 0;
				cCohort = 0;
				numCohorts++;
			}
			if (cCohort && (cSWindow->cohorts.size() >= hard_limit || (grammar->delimiters && doesTagMatchSet(cCohort->wordform, grammar->delimiters)))) {
				if (cSWindow->cohorts.size() >= hard_limit) {
					u_fprintf(ux_stderr, "Warning: Hard limit of %u cohorts reached at line %u - forcing break.\n", hard_limit, numLines);
					u_fflush(ux_stderr);
				}
				if (cCohort->readings.empty()) {
					cReading = initEmptyCohort(cCohort);
					lReading = cReading;
				}
				foreach (std::list<Reading*>, cCohort->readings, iter, iter_end) {
					addTagToReading(*iter, endtag);
				}

				cSWindow->appendCohort(cCohort);
				gWindow->appendSingleWindow(cSWindow);
				lSWindow = cSWindow;
				lCohort = cCohort;
				cSWindow = 0;
				cCohort = 0;
				numCohorts++;
			}
			if (!cSWindow) {
				// ToDo: Refactor to allocate SingleWindow, Cohort, and Reading from their containers
				cSWindow = new SingleWindow(gWindow);

				cCohort = r->new_Cohort(cSWindow);
				cCohort->global_number = 0;
				cCohort->wordform = begintag;

				cReading = r->new_Reading(cCohort);
				cReading->baseform = begintag;
				cReading->wordform = begintag;
				if (grammar->sets_any && !grammar->sets_any->empty()) {
					cReading->parent->possible_sets.insert(grammar->sets_any->begin(), grammar->sets_any->end());
					cReading->possible_sets.insert(grammar->sets_any->begin(), grammar->sets_any->end());
				}
				addTagToReading(cReading, begintag);

				cCohort->appendReading(cReading);

				cSWindow->appendCohort(cCohort);

				lSWindow = cSWindow;
				lReading = cReading;
				lCohort = cCohort;
				cCohort = 0;
				numWindows++;
			}
			if (cCohort && cSWindow) {
				cSWindow->appendCohort(cCohort);
				lCohort = cCohort;
				if (cCohort->readings.empty()) {
					cReading = initEmptyCohort(cCohort);
					lReading = cReading;
				}
			}
			if (gWindow->next.size() > num_windows) {
				while (!gWindow->previous.empty() && gWindow->previous.size() > num_windows) {
					SingleWindow *tmp = gWindow->previous.front();
					printSingleWindow(tmp, output);
					delete tmp;
					gWindow->previous.pop_front();
				}
				gWindow->shuffleWindowsDown();
				runGrammarOnWindow();
				if (numWindows % resetAfter == 0) {
					resetIndexes();
					r->trim();
				}
				if (verbosity_level > 0) {
					u_fprintf(ux_stderr, "Progress: L:%u, W:%u, C:%u, R:%u\r", lines, numWindows, numCohorts, numReadings);
					u_fflush(ux_stderr);
				}
			}
			cCohort = r->new_Cohort(cSWindow);
			cCohort->global_number = gWindow->cohort_counter++;
			cCohort->wordform = addTag(cleaned)->hash;
			lCohort = cCohort;
			lReading = 0;
			numCohorts++;
		}
		else if (cleaned[0] == ' ' && cleaned[1] == '"' && cCohort) {
			cReading = r->new_Reading(cCohort);
			cReading->wordform = cCohort->wordform;
			if (grammar->sets_any && !grammar->sets_any->empty()) {
				cReading->parent->possible_sets.insert(grammar->sets_any->begin(), grammar->sets_any->end());
				cReading->possible_sets.insert(grammar->sets_any->begin(), grammar->sets_any->end());
			}
			addTagToReading(cReading, cReading->wordform);

			ux_trim(cleaned);
			UChar *space = cleaned;
			UChar *base = space;
			if (*space == '"') {
				space++;
				SKIPTO_NOSPAN(&space, '"');
			}

			TagList mappings;

			while (space && (space = u_strchr(space, ' ')) != 0) {
				space[0] = 0;
				space++;
				if (base && base[0]) {
					Tag *tag = addTag(base);
					if (tag->type & T_MAPPING || tag->tag[0] == grammar->mapping_prefix) {
						mappings.push_back(tag);
					}
					else {
						addTagToReading(cReading, tag->hash);
					}
				}
				base = space;
				if (*space == '"') {
					space++;
					SKIPTO_NOSPAN(&space, '"');
				}
			}
			if (base && base[0]) {
				Tag *tag = addTag(base);
				if (tag->type & T_MAPPING || tag->tag[0] == grammar->mapping_prefix) {
					mappings.push_back(tag);
				}
				else {
					addTagToReading(cReading, tag->hash);
				}
			}
			if (!cReading->baseform) {
				u_fprintf(ux_stderr, "Warning: Line %u had no valid baseform.\n", numLines);
				u_fflush(ux_stderr);
			}
			if (!mappings.empty()) {
				splitMappings(mappings, cCohort, cReading, true);
			}
			cCohort->appendReading(cReading);
			lReading = cReading;
			numReadings++;
		}
		else {
			if (!ignoreinput && cleaned[0] == ' ' && cleaned[1] == '"') {
				if (verbosity_level > 0) {
					u_fprintf(ux_stderr, "Warning: Line %u looked like a reading but there was no containing cohort - treated as plain text.\n", numLines);
					u_fflush(ux_stderr);
				}
			}
			ux_trim(cleaned);
			if (u_strlen(cleaned) > 0) {
				if (u_strcmp(cleaned, stringbits[S_CMD_FLUSH]) == 0) {
					u_fprintf(ux_stderr, "Info: CGCMD:FLUSH encountered on line %u. Flushing...\n", numLines);
					if (cCohort && cSWindow) {
						cSWindow->appendCohort(cCohort);
						if (cCohort->readings.empty()) {
							cReading = initEmptyCohort(cCohort);
							lReading = cReading;
						}
						foreach (std::list<Reading*>, cCohort->readings, iter, iter_end) {
							addTagToReading(*iter, endtag);
						}
						gWindow->appendSingleWindow(cSWindow);
						cReading = lReading = 0;
						cCohort = lCohort = 0;
						cSWindow = lSWindow = 0;
					}
					while (!gWindow->next.empty()) {
						while (!gWindow->previous.empty() && gWindow->previous.size() > num_windows) {
							SingleWindow *tmp = gWindow->previous.front();
							printSingleWindow(tmp, output);
							delete tmp;
							gWindow->previous.pop_front();
						}
						gWindow->shuffleWindowsDown();
						runGrammarOnWindow();
						if (numWindows % resetAfter == 0) {
							resetIndexes();
							r->trim();
						}
						if (verbosity_level > 0) {
							u_fprintf(ux_stderr, "Progress: L:%u, W:%u, C:%u, R:%u\r", lines, numWindows, numCohorts, numReadings);
							u_fflush(ux_stderr);
						}
					}
					gWindow->shuffleWindowsDown();
					while (!gWindow->previous.empty()) {
						SingleWindow *tmp = gWindow->previous.front();
						printSingleWindow(tmp, output);
						delete tmp;
						gWindow->previous.pop_front();
					}
					u_fflush(output);
				}
				else if (u_strcmp(cleaned, stringbits[S_CMD_IGNORE]) == 0) {
					u_fprintf(ux_stderr, "Info: CGCMD:IGNORE encountered on line %u. Passing through all input...\n", numLines);
					ignoreinput = true;
				}
				else if (u_strcmp(cleaned, stringbits[S_CMD_RESUME]) == 0) {
					u_fprintf(ux_stderr, "Info: CGCMD:RESUME encountered on line %u. Resuming CG...\n", numLines);
					ignoreinput = false;
				}
				else if (u_strcmp(cleaned, stringbits[S_CMD_EXIT]) == 0) {
					u_fprintf(ux_stderr, "Info: CGCMD:EXIT encountered on line %u. Exiting...\n", numLines);
					u_fprintf(output, "%S", line);
					goto CGCMD_EXIT;
				}
				
				if (lReading && lCohort) {
					lCohort->text_post = ux_append(lCohort->text_post, line);
				}
				else if (lCohort) {
					lCohort->text_pre = ux_append(lCohort->text_pre, line);
				}
				else if (lSWindow) {
					lSWindow->text = ux_append(lSWindow->text, line);
				}
				else {
					u_fprintf(output, "%S", line);
				}
			}
		}
		numLines++;
		line[0] = cleaned[0] = 0;
	}

	if (cCohort && cSWindow) {
		cSWindow->appendCohort(cCohort);
		if (cCohort->readings.empty()) {
			cReading = initEmptyCohort(cCohort);
		}
		foreach (std::list<Reading*>, cCohort->readings, iter, iter_end) {
			addTagToReading(*iter, endtag);
		}
		gWindow->appendSingleWindow(cSWindow);
		cReading = 0;
		cCohort = 0;
		cSWindow = 0;
	}
	while (!gWindow->next.empty()) {
		while (!gWindow->previous.empty() && gWindow->previous.size() > num_windows) {
			SingleWindow *tmp = gWindow->previous.front();
			printSingleWindow(tmp, output);
			delete tmp;
			gWindow->previous.pop_front();
		}
		gWindow->shuffleWindowsDown();
		runGrammarOnWindow();
		if (verbosity_level > 0) {
			u_fprintf(ux_stderr, "Progress: L:%u, W:%u, C:%u, R:%u\r", lines, numWindows, numCohorts, numReadings);
			u_fflush(ux_stderr);
		}
	}

	gWindow->shuffleWindowsDown();
	while (!gWindow->previous.empty()) {
		SingleWindow *tmp = gWindow->previous.front();
		printSingleWindow(tmp, output);
		delete tmp;
		gWindow->previous.pop_front();
	}

	u_fflush(output);

CGCMD_EXIT:
	ticks tmp = getticks();
	grammar->total_time = elapsed(tmp, timer);
	if (verbosity_level > 0) {
		u_fprintf(ux_stderr, "Did %u lines, %u windows, %u cohorts, %u readings.\n", numLines, numWindows, numCohorts, numReadings);
		u_fflush(ux_stderr);
	}

	return 0;
}

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

#include "PlaintextApplicator.hpp"
#include "Strings.h"
#include "Tag.h"
#include "Grammar.h"
#include "Window.h"
#include "SingleWindow.h"
#include "Reading.h"

namespace CG3 {

PlaintextApplicator::PlaintextApplicator(UFILE *ux_err)
	: GrammarApplicator(ux_err)
{
}

void PlaintextApplicator::runGrammarOnText(istream& input, UFILE *output) {
	if (!input.good()) {
		u_fprintf(ux_stderr, "Error: Input is null - nothing to parse!\n");
		CG3Quit(1);
	}
	if (input.eof()) {
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

	if (!grammar->delimiters || grammar->delimiters->empty()) {
		if (!grammar->soft_delimiters || grammar->soft_delimiters->empty()) {
			u_fprintf(ux_stderr, "Warning: No soft or hard delimiters defined in grammar. Hard limit of %u cohorts may break windows in unintended places.\n", hard_limit);
		}
		else {
			u_fprintf(ux_stderr, "Warning: No hard delimiters defined in grammar. Soft limit of %u cohorts may break windows in unintended places.\n", soft_limit);
		}
	}

	std::vector<UChar> line(1024, 0);
	std::vector<UChar> cleaned(line.size(), 0);
	bool ignoreinput = false;
	bool did_soft_lookback = false;

	index();

	uint32_t resetAfter = ((num_windows+4)*2+1);
	uint32_t lines = 0;

	SingleWindow *cSWindow = 0;
	Cohort *cCohort = 0;
	Reading *cReading = 0;

	SingleWindow *lSWindow = 0;
	Cohort *lCohort = 0;
	Reading *lReading = 0;

	gWindow->window_span = num_windows;

	while (!input.eof()) {
		++lines;
		size_t offset = 0, packoff = 0;
		// Read as much of the next line as will fit in the current buffer
		while (input.gets(&line[offset], line.size()-offset-1)) {
			// Copy the segment just read to cleaned
			for (size_t i=offset ; i<line.size() ; ++i) {
				// Only copy one space character, regardless of how many are in input
				if (ISSPACE(line[i]) && !ISNL(line[i])) {
					cleaned[packoff++] = ' ';
					while (ISSPACE(line[i]) && !ISNL(line[i])) {
						++i;
					}
				}
				// Break if there is a newline
				if (ISNL(line[i])) {
					cleaned[packoff+1] = cleaned[packoff] = 0;
					goto gotaline; // Oh how I wish C++ had break 2;
				}
				if (line[i] == 0) {
					cleaned[packoff+1] = cleaned[packoff] = 0;
					break;
				}
				cleaned[packoff++] = line[i];
			}
			// If we reached this, buffer wasn't big enough. Double the size of the buffer and try again.
			offset = line.size()-2;
			line.resize(line.size()*2, 0);
			cleaned.resize(line.size()+1, 0);
		}

gotaline:
		// Trim trailing whitespace
		while (cleaned[0] && ISSPACE(cleaned[packoff-1])) {
			cleaned[packoff-1] = 0;
			--packoff;
		}
		if (!ignoreinput && cleaned[0] && cleaned[0] != '<') {
			if (cCohort && cCohort->readings.empty()) {
				cReading = initEmptyCohort(*cCohort);
				lReading = cReading;
			}
			if (cSWindow && cSWindow->cohorts.size() >= soft_limit && grammar->soft_delimiters && !did_soft_lookback) {
				did_soft_lookback = true;
				reverse_foreach (CohortVector, cSWindow->cohorts, iter, iter_end) {
					if (doesSetMatchCohortNormal(**iter, grammar->soft_delimiters->hash)) {
						did_soft_lookback = false;
						Cohort *cohort = delimitAt(*cSWindow, *iter);
						cSWindow = cohort->parent->next;
						if (cCohort) {
							cCohort->parent = cSWindow;
						}
						if (verbosity_level > 0) {
							u_fprintf(ux_stderr, "Warning: Soft limit of %u cohorts reached at line %u but found suitable soft delimiter in buffer.\n", soft_limit, numLines);
							u_fflush(ux_stderr);
						}
						break;
					}
				}
			}
			if (cCohort && cSWindow->cohorts.size() >= soft_limit && grammar->soft_delimiters && doesSetMatchCohortNormal(*cCohort, grammar->soft_delimiters->hash)) {
				if (verbosity_level > 0) {
					u_fprintf(ux_stderr, "Warning: Soft limit of %u cohorts reached at line %u but found suitable soft delimiter.\n", soft_limit, numLines);
					u_fflush(ux_stderr);
				}
				foreach (ReadingList, cCohort->readings, iter, iter_end) {
					addTagToReading(**iter, endtag);
				}

				cSWindow->appendCohort(cCohort);
				lSWindow = cSWindow;
				lCohort = cCohort;
				cSWindow = 0;
				cCohort = 0;
				numCohorts++;
				did_soft_lookback = false;
			}
			if (cCohort && (cSWindow->cohorts.size() >= hard_limit || (!dep_delimit && grammar->delimiters && doesSetMatchCohortNormal(*cCohort, grammar->delimiters->hash)))) {
				if (cSWindow->cohorts.size() >= hard_limit) {
					u_fprintf(ux_stderr, "Warning: Hard limit of %u cohorts reached at line %u - forcing break.\n", hard_limit, numLines);
					u_fflush(ux_stderr);
				}
				foreach (ReadingList, cCohort->readings, iter, iter_end) {
					addTagToReading(**iter, endtag);
				}

				cSWindow->appendCohort(cCohort);
				lSWindow = cSWindow;
				lCohort = cCohort;
				cSWindow = 0;
				cCohort = 0;
				numCohorts++;
				did_soft_lookback = false;
			}
			if (!cSWindow) {
				// ToDo: Refactor to allocate SingleWindow, Cohort, and Reading from their containers
				cSWindow = gWindow->allocAppendSingleWindow();
				initEmptySingleWindow(cSWindow);

				lSWindow = cSWindow;
				lReading = cSWindow->cohorts[0]->readings.front();
				lCohort = cSWindow->cohorts[0];
				cCohort = 0;
				numWindows++;
				did_soft_lookback = false;
			}
			if (cCohort && cSWindow) {
				cSWindow->appendCohort(cCohort);
				lCohort = cCohort;
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
				}
				if (verbosity_level > 0) {
					u_fprintf(ux_stderr, "Progress: L:%u, W:%u, C:%u, R:%u\r", lines, numWindows, numCohorts, numReadings);
					u_fflush(ux_stderr);
				}
			}
			UString tag;
			UChar *base = &cleaned[0];
			UChar *space = base;

			while (space && *space && (space = u_strchr(space, ' ')) != 0) {
				space[0] = 0;
				if (base && base[0]) {
					cCohort = new Cohort(cSWindow);
					cCohort->global_number = gWindow->cohort_counter++;
					tag.clear();
					tag += '"';
					tag += '<';
					tag += base;
					tag += '>';
					tag += '"';
					cCohort->wordform = addTag(tag)->hash;
					lCohort = cCohort;
					numCohorts++;
					cReading = initEmptyCohort(*cCohort);
					lReading = cReading;
					cSWindow->appendCohort(cCohort);
					cCohort = 0;
				}
				base = ++space;
			}
			if (base && base[0]) {
				cCohort = new Cohort(cSWindow);
				cCohort->global_number = gWindow->cohort_counter++;
				tag.clear();
				tag += '"';
				tag += '<';
				tag += base;
				tag += '>';
				tag += '"';
				cCohort->wordform = addTag(tag)->hash;
				lCohort = cCohort;
				numCohorts++;
				cReading = initEmptyCohort(*cCohort);
				lReading = cReading;
				cSWindow->appendCohort(cCohort);
				cCohort = 0;
			}
		}
		else {
			if (cleaned[0] && line[0]) {
				if (lCohort) {
					lCohort->text += &line[0];
				}
				else if (lSWindow) {
					lSWindow->text += &line[0];
				}
				else {
					u_fprintf(output, "%S", &line[0]);
				}
			}
		}
		numLines++;
		line[0] = cleaned[0] = 0;
	}

	if (cCohort && cSWindow) {
		cSWindow->appendCohort(cCohort);
		if (cCohort->readings.empty()) {
			cReading = initEmptyCohort(*cCohort);
		}
		foreach (ReadingList, cCohort->readings, iter, iter_end) {
			addTagToReading(**iter, endtag);
		}
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

}
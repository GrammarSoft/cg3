/*
* Copyright (C) 2007-2018, GrammarSoft ApS
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
#include "Strings.hpp"
#include "Tag.hpp"
#include "Grammar.hpp"
#include "Window.hpp"
#include "SingleWindow.hpp"
#include "Reading.hpp"

namespace CG3 {

PlaintextApplicator::PlaintextApplicator(std::ostream& ux_err)
  : GrammarApplicator(ux_err)
{
	allow_magic_readings = true;
}

void PlaintextApplicator::runGrammarOnText(std::istream& input, std::ostream& output) {
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

	uint32_t resetAfter = ((num_windows + 4) * 2 + 1);
	uint32_t lines = 0;

	SingleWindow* cSWindow = 0;
	Cohort* cCohort = 0;
	Reading* cReading = 0;

	SingleWindow* lSWindow = 0;
	Cohort* lCohort = 0;

	gWindow->window_span = num_windows;

	ux_stripBOM(input);

	while (!input.eof()) {
		++lines;
		size_t offset = 0, packoff = 0;
		// Read as much of the next line as will fit in the current buffer
		while (u_fgets(&line[offset], line.size() - offset - 1, input)) {
			// Copy the segment just read to cleaned
			for (size_t i = offset; i < line.size(); ++i) {
				// Only copy one space character, regardless of how many are in input
				if (ISSPACE(line[i]) && !ISNL(line[i])) {
					cleaned[packoff++] = ' ';
					while (ISSPACE(line[i]) && !ISNL(line[i])) {
						++i;
					}
				}
				// Break if there is a newline
				if (ISNL(line[i])) {
					cleaned[packoff + 1] = cleaned[packoff] = 0;
					goto gotaline; // Oh how I wish C++ had break 2;
				}
				if (line[i] == 0) {
					cleaned[packoff + 1] = cleaned[packoff] = 0;
					break;
				}
				cleaned[packoff++] = line[i];
			}
			// If we reached this, buffer wasn't big enough. Double the size of the buffer and try again.
			offset = line.size() - 2;
			line.resize(line.size() * 2, 0);
			cleaned.resize(line.size() + 1, 0);
		}

	gotaline:
		// Trim trailing whitespace
		while (cleaned[0] && ISSPACE(cleaned[packoff - 1])) {
			cleaned[packoff - 1] = 0;
			--packoff;
		}
		if (!ignoreinput && cleaned[0] && cleaned[0] != '<') {
			if (cCohort && cCohort->readings.empty()) {
				initEmptyCohort(*cCohort);
			}
			if (cSWindow && cSWindow->cohorts.size() >= soft_limit && grammar->soft_delimiters && !did_soft_lookback) {
				did_soft_lookback = true;
				reverse_foreach (iter, cSWindow->cohorts) {
					if (doesSetMatchCohortNormal(**iter, grammar->soft_delimiters->number)) {
						did_soft_lookback = false;
						Cohort* cohort = delimitAt(*cSWindow, *iter);
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
			if (cCohort && cSWindow->cohorts.size() >= soft_limit && grammar->soft_delimiters && doesSetMatchCohortNormal(*cCohort, grammar->soft_delimiters->number)) {
				if (verbosity_level > 0) {
					u_fprintf(ux_stderr, "Warning: Soft limit of %u cohorts reached at line %u but found suitable soft delimiter.\n", soft_limit, numLines);
					u_fflush(ux_stderr);
				}
				for (auto iter : cCohort->readings) {
					addTagToReading(*iter, endtag);
				}

				cSWindow->appendCohort(cCohort);
				lSWindow = cSWindow;
				lCohort = cCohort;
				cSWindow = 0;
				cCohort = 0;
				numCohorts++;
				did_soft_lookback = false;
			}
			if (cCohort && (cSWindow->cohorts.size() >= hard_limit || (!dep_delimit && grammar->delimiters && doesSetMatchCohortNormal(*cCohort, grammar->delimiters->number)))) {
				if (!is_conv && cSWindow->cohorts.size() >= hard_limit) {
					u_fprintf(ux_stderr, "Warning: Hard limit of %u cohorts reached at line %u - forcing break.\n", hard_limit, numLines);
					u_fflush(ux_stderr);
				}
				for (auto iter : cCohort->readings) {
					addTagToReading(*iter, endtag);
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
				lCohort = cSWindow->cohorts[0];
				cCohort = 0;
				numWindows++;
				did_soft_lookback = false;
			}
			if (gWindow->next.size() > num_windows) {
				while (!gWindow->previous.empty() && gWindow->previous.size() > num_windows) {
					SingleWindow* tmp = gWindow->previous.front();
					printSingleWindow(tmp, output);
					free_swindow(tmp);
					gWindow->previous.erase(gWindow->previous.begin());
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
			std::vector<UChar*> tokens_raw;
			UChar* base = &cleaned[0];
			UChar* space = base;

			while (space && *space && (space = u_strchr(space, ' ')) != 0) {
				space[0] = 0;
				if (base && base[0]) {
					tokens_raw.push_back(base);
				}
				base = ++space;
			}
			if (base && base[0]) {
				tokens_raw.push_back(base);
			}

			std::vector<UnicodeString> tokens;
			for (size_t i = 0; i < tokens_raw.size(); ++i) {
				UChar* p = tokens_raw[i];
				size_t len = u_strlen(p);
				while (*p && u_ispunct(p[0])) {
					tokens.push_back(UnicodeString(p[0]));
					++p;
					--len;
				}
				size_t tkz = tokens.size();
				while (*p && u_ispunct(p[len - 1])) {
					tokens.push_back(UnicodeString(p[len - 1]));
					p[len - 1] = 0;
					--len;
				}
				if (*p) {
					tokens.insert(tokens.begin() + tkz, p);
				}
			}

			UString tag;
			for (size_t i = 0; i < tokens.size(); ++i) {
				UnicodeString& token = tokens[i];
				bool first_upper = (u_isupper(token[0]) != 0);
				bool all_upper = first_upper;
				bool mixed_upper = false;
				for (int32_t i = 1; i < token.length(); ++i) {
					if (u_isupper(token[i])) {
						mixed_upper = true;
					}
					else {
						all_upper = false;
					}
				}

				cCohort = alloc_cohort(cSWindow);
				cCohort->global_number = gWindow->cohort_counter++;
				tag.clear();
				tag += '"';
				tag += '<';
				tag += token.getTerminatedBuffer();
				tag += '>';
				tag += '"';
				cCohort->wordform = addTag(tag);
				lCohort = cCohort;
				numCohorts++;
				cReading = initEmptyCohort(*cCohort);
				cReading->noprint = !add_tags;
				if (add_tags) {
					constexpr char _tag[] = "<cg-conv>";
					tag.assign(_tag, _tag + sizeof(_tag) - 1);
					addTagToReading(*cReading, addTag(tag));
				}
				if (add_tags && (first_upper || all_upper || mixed_upper)) {
					delTagFromReading(*cReading, cReading->baseform);
					token.toLower();
					tag.clear();
					tag += '"';
					tag += token.getTerminatedBuffer();
					tag += '"';
					addTagToReading(*cReading, addTag(tag));
					if (all_upper) {
						constexpr char _tag[] = "<all-upper>";
						tag.assign(_tag, _tag + sizeof(_tag) - 1);
						addTagToReading(*cReading, addTag(tag));
					}
					if (first_upper) {
						constexpr char _tag[] = "<first-upper>";
						tag.assign(_tag, _tag + sizeof(_tag) - 1);
						addTagToReading(*cReading, addTag(tag));
					}
					if (mixed_upper && !all_upper) {
						constexpr char _tag[] = "<mixed-upper>";
						tag.assign(_tag, _tag + sizeof(_tag) - 1);
						addTagToReading(*cReading, addTag(tag));
					}
				}
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
			initEmptyCohort(*cCohort);
		}
		for (auto iter : cCohort->readings) {
			addTagToReading(*iter, endtag);
		}
		cReading = 0;
		cCohort = 0;
		cSWindow = 0;
	}
	while (!gWindow->next.empty()) {
		while (!gWindow->previous.empty() && gWindow->previous.size() > num_windows) {
			SingleWindow* tmp = gWindow->previous.front();
			printSingleWindow(tmp, output);
			free_swindow(tmp);
			gWindow->previous.erase(gWindow->previous.begin());
		}
		gWindow->shuffleWindowsDown();
		runGrammarOnWindow();
	}

	gWindow->shuffleWindowsDown();
	while (!gWindow->previous.empty()) {
		SingleWindow* tmp = gWindow->previous.front();
		printSingleWindow(tmp, output);
		free_swindow(tmp);
		gWindow->previous.erase(gWindow->previous.begin());
	}

	u_fflush(output);
}

void PlaintextApplicator::printCohort(Cohort* cohort, std::ostream& output) {
	if (cohort->local_number == 0) {
		return;
	}
	if (cohort->type & CT_REMOVED) {
		return;
	}

	u_fprintf(output, "%.*S ", cohort->wordform->tag.size() - 4, cohort->wordform->tag.c_str() + 2);
}

void PlaintextApplicator::printSingleWindow(SingleWindow* window, std::ostream& output) {
	uint32_t cs = (uint32_t)window->cohorts.size();
	for (uint32_t c = 0; c < cs; c++) {
		Cohort* cohort = window->cohorts[c];
		printCohort(cohort, output);
	}
	u_fputc('\n', output);
	u_fflush(output);
}
}
